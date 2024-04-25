#ifndef TYPE_UNIQUELIST
#define TYPE_UNIQUELIST

#include <algorithm>
#include <map>
#include <utility>

namespace Type
{

template <typename T> class UniqueList
{
	struct links_t;
	using container_type = std::map<T, links_t>;

	using link_t = typename container_type::pointer;
	using const_link_t = typename container_type::const_pointer;

	struct links_t
	{
		link_t pre{};
		link_t post{};
	};

	template <bool forward = true> struct iterator
	{
		const_link_t ptr;
		[[nodiscard]] const T &operator*() const noexcept
		{
			return ptr->first;
		}
		iterator &operator++() noexcept
		{
			if constexpr (forward) { ptr = ptr->second.post; }
			else {
				ptr = ptr->second.pre;
			}
			return *this;
		}
		[[nodiscard]] iterator operator++(int) noexcept
		{
			auto tmp = *this;
			++*this;
			return tmp;
		}
		[[nodiscard]] bool operator==(
		    const iterator &other) const noexcept = default;
		[[nodiscard]] bool operator!=(
		    const iterator &other) const noexcept = default;

		using difference_type = std::ptrdiff_t;
		using value_type = T;
		using pointer = const T *;
		using reference = const T &;
		using iterator_category = std::forward_iterator_tag;
	};

	void after_push_back(
	    const typename container_type::iterator &item) noexcept
	{
		if (auto &&preLast =
		        std::exchange(last, std::to_address(item)))
			preLast->second.post = last;
		else
			first = last;
	}

	void insert(typename container_type::node_type &&node) noexcept
	{
		auto &&it = items.insert(std::move(node)).position;
		it->second = {last};
		after_push_back(it);
	}

	void before_remove(links_t &ptr) noexcept
	{
		if (ptr.pre)
			ptr.pre->second.post = ptr.post;
		else
			first = ptr.post;

		if (ptr.post)
			ptr.post->second.pre = ptr.pre;
		else
			last = ptr.pre;
	}

	typename container_type::node_type extract(const T &val) noexcept
	{
		auto &&it = items.find(val);
		before_remove(it->second);
		return items.extract(it);
	}

public:
	bool push_back(const T &value)
	{
		auto &&[it, newly] = items.try_emplace(value, last);
		if (!newly) return false;
		after_push_back(it);
		return true;
	}

	bool push_front(const T &value)
	{
		auto &&[it, newly] = items.try_emplace(value, nullptr, first);
		if (!newly) return false;
		if (auto &&preFirst =
		        std::exchange(first, std::to_address(it)))
			preFirst->second.pre = first;
		else
			last = first;
		return true;
	}

	[[nodiscard]] iterator<> begin() const noexcept
	{
		return {first};
	}
	[[nodiscard]] static iterator<> end() noexcept { return {}; }
	[[nodiscard]] iterator<false> rbegin() const noexcept
	{
		return {last};
	}
	[[nodiscard]] static iterator<false> rend() noexcept
	{
		return {};
	}

	[[nodiscard]] bool empty() const noexcept { return !first; }
	void clear() noexcept
	{
		first = last = {};
		items.clear();
	}

	[[nodiscard]] size_t size() const noexcept
	{
		return items.size();
	}

	bool remove(const T &value) noexcept
	{
		if (auto it = items.find(value); it != items.end()) {
			before_remove(it->second);
			items.erase(it);
			return true;
		}
		return false;
	}

	[[nodiscard]] bool operator==(
	    const UniqueList &rhs) const noexcept
	{
		return size() == rhs.size()
		    && std::equal(begin(), end(), rhs.begin());
	}

	[[nodiscard]] bool contains(const T &item) const noexcept
	{
		return items.contains(item);
	}

	[[nodiscard]] UniqueList split_by(const UniqueList &by) noexcept
	{
		UniqueList common;
		for (auto it = begin(); it != end();)
			if (auto &val = *it++; by.contains(val))
				common.insert(extract(val));
		return common;
	}

	UniqueList() noexcept = default;
	UniqueList(UniqueList &&) noexcept = default;
	UniqueList &operator=(UniqueList &&) noexcept = default;
	UniqueList(const UniqueList &other)
	{
		for (auto &&item : other) push_back(item);
	}
	UniqueList &operator=(const UniqueList &other)
	{
		if (this == &other) return *this;
		clear();
		for (auto &&item : other) push_back(item);
		return *this;
	}

private:
	container_type items;
	link_t first{};
	link_t last{};
};

}

#endif
