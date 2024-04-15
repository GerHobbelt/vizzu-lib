
#include "data_source.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <utility>
#include <vector>

#include "../old/datatable.h"
#include "base/text/naturalcmp.h"

#include "aggregators.h"
#include "dataframe.h"

namespace Vizzu::dataframe
{

template <bool /*stable*/ = false, bool /*sparse*/ = true>
struct index_erase_if
{
	std::span<const std::size_t> indices;

	template <class Cont>
	constexpr void operator()(Cont &&cont) const noexcept;
};

template <>
template <class Cont>
constexpr void index_erase_if<true>::operator()(
    Cont &&cont) const noexcept
{
	if (indices.empty()) return;
	auto into = cont.begin() + indices.front();
	auto prev = into + 1;

	for (std::size_t i{1}; i < indices.size(); ++i) {
		auto curr = cont.begin() + indices[i];
		into = std::move(std::exchange(prev, curr + 1), curr, into);
	}
	cont.erase(std::move(prev, cont.end(), into), cont.end());
}

template <>
template <class Cont>
constexpr void index_erase_if<>::operator()(
    Cont &&cont) const noexcept
{
	auto from = cont.end();
	for (auto i : std::ranges::views::reverse(indices))
		cont[i] = std::move(*--from);
	cont.erase(from, cont.end());
}

template <>
template <class Cont>
constexpr void index_erase_if<true, false>::operator()(
    Cont &&cont) const noexcept
{
	std::erase_if(cont,
	    [i = std::size_t{}, c = indices.begin(), e = indices.end()](
	        const auto &) mutable
	    {
		    return c != e && i++ == *c && (++c, true);
	    });
}

template <>
template <class Cont>
constexpr void index_erase_if<false, false>::operator()(
    Cont &&cont) const noexcept
{
	auto from_ix = cont.size();
	for (auto first = indices.begin(),
	          last = std::lower_bound(first,
	              indices.end(),
	              from_ix - indices.size()),
	          from = indices.end();
	     first != last;
	     cont[*first++] = std::move(cont[from_ix]))
		while (--from_ix == from[-1]) --from;
}

std::size_t data_source::get_record_count() const
{
	if (!finalized.empty()) return finalized.size();

	std::size_t record_count{};
	for (const auto &dim : dimensions)
		if (record_count < dim.values.size())
			record_count = dim.values.size();

	for (const auto &mea : measures)
		if (record_count < mea.values.size())
			record_count = mea.values.size();

	return record_count;
}

struct data_source::sorter
{
	[[nodiscard]] static std::weak_ordering
	cmp_dim(std::uint32_t lhs, std::uint32_t rhs, na_position na)
	{
		return na == na_position::last || (lhs != nav && rhs != nav)
		         ? lhs <=> rhs
		         : rhs <=> lhs;
	}

	[[nodiscard]] static std::weak_ordering
	cmp_mea(double lhs, double rhs, na_position na, sort_type sort)
	{
		using std::weak_order;
		return std::isnan(lhs) || std::isnan(rhs)
		         ? na == na_position::last ? weak_order(lhs, rhs)
		                                   : weak_order(rhs, lhs)
		     : sort == sort_type::less ? weak_order(lhs, rhs)
		                               : weak_order(rhs, lhs);
	}

	[[nodiscard]] static std::weak_ordering
	cmp(const sort_one_series &sorter, std::size_t a, std::size_t b)
	{
		switch (auto &&[series, sort, na] = sorter; series) {
			using enum series_type;
		case dimension: {
			const auto &dim = unsafe_get<dimension>(series).second;
			return cmp_dim(dim.values[a], dim.values[b], na);
		}
		case measure: {
			const auto &mea = unsafe_get<measure>(series).second;
			return cmp_mea(mea.values[a], mea.values[b], na, sort);
		}
		default:
		case unknown: return std::weak_ordering::equivalent;
		}
	}
};

std::vector<std::size_t> data_source::get_sorted_indices(
    const sorting_type &sorters) const
{
	thread_local const sorting_type *sorters_ptr{};
	sorters_ptr = &sorters;

	return data_source::get_sorted_indices(get_record_count(),
	    [](std::size_t a, std::size_t b)
	    {
		    for (const auto &sorter : *sorters_ptr) {
			    if (auto res = sorter::cmp(sorter, a, b); is_neq(res))
				    return is_lt(res);
		    }
		    return false;
	    });
}

void data_source::sort(std::vector<std::size_t> &&indices)
{
	std::vector<double> tmp_mea(measures.size());
	std::vector<std::uint32_t> tmp_dim(dimensions.size());

	for (std::size_t i{}, max = indices.size(); i < max; ++i) {
		if (i >= indices[i]) continue;

		for (std::size_t m{}; m < measures.size(); ++m)
			tmp_mea[m] = measures[m].values[i];
		for (std::size_t d{}; d < dimensions.size(); ++d)
			tmp_dim[d] = dimensions[d].values[i];

		std::size_t j{i};
		for (; indices[j] != i; j = std::exchange(indices[j], i)) {
			for (auto &meas : measures)
				meas.values[j] = meas.values[indices[j]];
			for (auto &dims : dimensions)
				dims.values[j] = dims.values[indices[j]];
		}
		for (std::size_t m{}; m < measures.size(); ++m)
			measures[m].values[j] = tmp_mea[m];
		for (std::size_t d{}; d < dimensions.size(); ++d)
			dimensions[d].values[j] = tmp_dim[d];
	}
}

std::size_t data_source::change_series_identifier_type(
    const std::string_view &name) const
{
	if (auto it = std::lower_bound(dimension_names.begin(),
	        dimension_names.end(),
	        name);
	    it != dimension_names.end() && *it == name)
		return static_cast<std::size_t>(it - dimension_names.begin());
	if (auto it = std::lower_bound(measure_names.begin(),
	        measure_names.end(),
	        name);
	    it != measure_names.end() && *it == name)
		return static_cast<std::size_t>(
		           std::distance(measure_names.begin(), it))
		     + dimension_names.size();
	return ~std::size_t{};
}

std::size_t data_source::change_record_identifier_type(
    const std::string &id) const
{
	auto it = finalized.find(id);
	return it != finalized.end() ? it->second : ~std::size_t{};
}

cell_value data_source::get_data(std::size_t record_id,
    const std::string_view &column) const
{
	switch (auto &&series = get_series(column)) {
		using enum series_type;
	case dimension: {
		const auto &dims = unsafe_get<dimension>(series).second;
		if (record_id >= dims.values.size())
			return std::string_view{};
		return dims.get(record_id);
	}
	case measure: {
		const auto &meas = unsafe_get<measure>(series).second;
		if (record_id >= meas.values.size()) return nan;

		return meas.values[record_id];
	}
	case unknown:
	default: throw;
	}
}

data_source::series_data data_source::get_series(
    const std::string_view &id)
{
	if (auto size = change_series_identifier_type(id);
	    size < dimensions.size())
		return series_data{std::in_place_index<1>,
		    dimension_names[size],
		    dimensions[size]};
	else if (size < dimensions.size() + measures.size())
		return series_data{std::in_place_index<2>,
		    measure_names[size - dimensions.size()],
		    measures[size - dimensions.size()]};
	return {};
}

data_source::const_series_data data_source::get_series(
    const std::string_view &id) const
{
	if (auto size = change_series_identifier_type(id);
	    size < dimensions.size())
		return const_series_data{std::in_place_index<1>,
		    dimension_names[size],
		    dimensions[size]};
	else if (size < dimensions.size() + measures.size())
		return const_series_data{std::in_place_index<2>,
		    measure_names[size - dimensions.size()],
		    measures[size - dimensions.size()]};

	return {};
}

void data_source::normalize_sizes()
{
	auto record_count = get_record_count();
	for (auto &&dim : dimensions)
		if (dim.values.size() < record_count) {
			dim.values.resize(record_count, nav);
			dim.contains_nav = true;
		}

	for (auto &&mea : measures)
		if (mea.values.size() < record_count) {
			mea.values.resize(record_count, nan);
			mea.contains_nan = true;
		}
}

void data_source::remove_series(std::span<const std::size_t> dims,
    std::span<const std::size_t> meas)
{
	const index_erase_if<true> dim_remover{dims};
	dim_remover(dimensions);
	dim_remover(dimension_names);
	const index_erase_if<true> mea_remover{meas};
	mea_remover(measures);
	mea_remover(measure_names);
}
void data_source::finalize()
{
	if (finalized.empty()) {
		normalize_sizes();
		const auto records = get_record_count();
		for (std::size_t r{}; r < records; ++r)
			if (!finalized.try_emplace(get_id(r, dimension_names), r)
			         .second)
				throw;
	}
}
data_source::dimension_t &data_source::add_new_dimension(
    std::span<const char *const> dimension_categories,
    std::span<const std::uint32_t> dimension_values,
    std::string_view name,
    std::span<const std::pair<const char *, const char *>> info)
{
	auto it = dimension_names.emplace(
	    std::lower_bound(dimension_names.begin(),
	        dimension_names.end(),
	        name),
	    name);
	return *dimensions.emplace(dimensions.begin()
	                               + (it - dimension_names.begin()),
	    dimension_categories,
	    dimension_values,
	    info);
}

data_source::dimension_t &data_source::add_new_dimension(
    dimension_t &&dim,
    std::string_view name)
{
	auto it = dimension_names.emplace(
	    std::lower_bound(dimension_names.begin(),
	        dimension_names.end(),
	        name),
	    name);
	return *dimensions.emplace(dimensions.begin()
	                               + (it - dimension_names.begin()),
	    std::move(dim));
}

data_source::measure_t &data_source::add_new_measure(
    std::span<const double> measure_values,
    std::string_view name,
    std::span<const std::pair<const char *, const char *>> info)
{
	auto it =
	    measure_names.emplace(std::lower_bound(measure_names.begin(),
	                              measure_names.end(),
	                              name),
	        name);
	return *measures.emplace(measures.begin()
	                             + (it - measure_names.begin()),
	    measure_values,
	    info);
}

data_source::measure_t &data_source::add_new_measure(measure_t &&mea,
    std::string_view name)
{
	auto it =
	    measure_names.emplace(std::lower_bound(measure_names.begin(),
	                              measure_names.end(),
	                              name),
	        name);
	return *measures.emplace(measures.begin()
	                             + (it - measure_names.begin()),
	    std::move(mea));
}

std::vector<std::size_t> data_source::get_sorted_indices(
    std::size_t max,
    bool (*sort)(std::size_t, std::size_t))
{
	std::vector<std::size_t> indices(max);
	std::iota(indices.begin(), indices.end(), std::size_t{});
	std::sort(indices.begin(), indices.end(), sort);
	return indices;
}

void data_source::remove_records(std::span<const std::size_t> indices)
{
	const index_erase_if<false> indices_remover{indices};
	for (auto &&dim : dimensions) {
		indices_remover(dim.values);
		dim.contains_nav = std::any_of(dim.values.begin(),
		    dim.values.end(),
		    std::bind_front(std::equal_to{}, nav));
	}
	for (auto &&mea : measures) {
		indices_remover(mea.values);
		mea.contains_nan = std::any_of(mea.values.begin(),
		    mea.values.end(),
		    static_cast<bool (&)(double)>(std::isnan));
	}
}

struct aggregating_helper
{
	std::size_t index;
	std::vector<custom_aggregator::id_type> aggregators;
};

data_source::data_source(aggregating_type &&aggregating,
    const std::vector<bool> *filtered,
    std::size_t record_count)
{
	auto &[dims, meas] = aggregating;

	dimensions.reserve(dims.size());
	dimension_names.reserve(dims.size());
	measures.reserve(meas.size());
	measure_names.reserve(meas.size());

	for (const auto &[name, dim] : dims)
		add_new_dimension({dim.get().categories,
		                      std::initializer_list<std::uint32_t>{},
		                      dim.get().info},
		    name);

	for (const auto &[name, mea] : meas) {
		measure_t &new_mea = add_new_measure({}, name);
		switch (const auto &ser = std::get<0>(mea)) {
			using enum series_type;
		case dimension:
			new_mea.info = unsafe_get<dimension>(ser).second.info;
			break;
		case measure:
			new_mea.info = unsafe_get<measure>(ser).second.info;
			break;
		default: break;
		}
	}

	std::vector<std::uint32_t> cat_indices(dims.size());
	std::map<std::vector<std::uint32_t>, aggregating_helper>
	    rec_to_index;

	for (std::size_t i{}; i < record_count; ++i) {
		if (filtered && (*filtered)[i]) continue;

		for (std::size_t ix{}; const auto &[name, dim] : dims)
			cat_indices[ix++] = i < dim.get().values.size()
			                      ? dim.get().values[i]
			                      : nav;

		auto [it, s] = rec_to_index.try_emplace(cat_indices,
		    rec_to_index.size());

		auto &[index, aggregators] = it->second;
		if (s) {
			for (auto &m : measures) m.values.emplace_back(nan);

			for (std::size_t ix{}; const auto &[name, dim] : dims)
				if (auto &newDim = dimensions[ix++];
				    newDim.values.emplace_back(
				        i < dim.get().values.size()
				            ? dim.get().values[i]
				            : nav)
				    == nav)
					newDim.contains_nav = true;

			aggregators.reserve(meas.size());
			for (const auto &[key, val] : meas)
				aggregators.emplace_back(val.second.create());
		}

		for (std::size_t ix{}; const auto &[name, mea] : meas) {
			const auto &[data, agg] = mea;
			cell_value val{};
			switch (data) {
				using enum series_type;
			case measure:
				val = unsafe_get<measure>(data).second.get(i);
				break;
			case dimension:
				val = unsafe_get<dimension>(data).second.get(i);
				break;
			default: break;
			}
			measures[ix].values[index] =
			    agg.add(aggregators[ix], val);
			++ix;
		}
	}

	for (auto &mea : measures)
		mea.contains_nan = std::any_of(mea.values.begin(),
		    mea.values.end(),
		    static_cast<bool (&)(double)>(std::isnan));
}

data_source::data_source(
    const std::shared_ptr<const data_source> &copying,
    std::optional<std::vector<bool>> &&filtered,
    std::optional<std::vector<std::size_t>> &&sorted) :
    measure_names(copying->measure_names),
    measures(copying->measures),
    dimension_names(copying->dimension_names),
    dimensions(copying->dimensions)
{
	if (sorted) { this->sort(std::move(*sorted)); }
	if (filtered) {
		const auto &filt = *filtered;
		std::vector<std::size_t> remove_ix;
		remove_ix.reserve(filt.size());
		for (std::size_t i{}; i < filt.size(); ++i)
			if (filt[i]) remove_ix.emplace_back(i);
		remove_records(remove_ix);
	}
}

std::string data_source::get_id(std::size_t record,
    std::span<const std::string> series) const
{
	std::string res;
	for (std::size_t ix{}; const auto &name : series) {
		while (dimension_names[ix] != name) ++ix;
		auto val = dimensions[ix].get(record);
		res += name;
		res += ':';
		res += val.data() == nullptr ? "__null__" : val;
		res += ';';
	}
	return res;
}

std::vector<std::size_t> data_source::dimension_t::get_indices(
    const dataframe_interface::any_sort_type &sorter) const
{
	thread_local const dataframe_interface::any_sort_type *sorts{};
	thread_local const std::vector<std::string> *cats{};
	sorts = &sorter;
	cats = &categories;
	return data_source::get_sorted_indices(categories.size(),
	    [](std::size_t a, std::size_t b)
	    {
		    static const Text::NaturalCmp cmp{};
		    switch (*sorts) {
		    default:
		    case sort_type::less: return (*cats)[a] < (*cats)[b];
		    case sort_type::greater: return (*cats)[b] < (*cats)[a];
		    case sort_type::natural_less:
			    return cmp((*cats)[a], (*cats)[b]);
		    case sort_type::natural_greater:
			    return cmp((*cats)[b], (*cats)[a]);
		    }
	    });
}

void data_source::dimension_t::sort_by(
    std::vector<std::size_t> &&indices,
    na_position na)
{
	for (auto &val : values)
		if (val != nav) val = indices[val];

	for (std::size_t i{}; i < categories.size(); ++i) {
		if (i >= indices[i]) continue;

		auto tmp = std::move(categories[i]);

		std::size_t j{i};
		for (; indices[j] != i; j = std::exchange(indices[j], i))
			categories[j] = std::move(categories[indices[j]]);

		categories[j] = std::move(tmp);
	}
	na_pos = na;
}

void data_source::dimension_t::add_element(
    std::string_view const &cat)
{
	if (values.emplace_back(get_or_set_cat(cat)) == nav)
		contains_nav = true;
}
void data_source::dimension_t::add_more_data(
    std::span<const char *const> new_categories,
    std::span<const std::uint32_t> new_values)
{
	std::vector<std::uint32_t> remap(new_categories.size());
	for (auto i = std::size_t{}; i < remap.size(); ++i) {
		remap[i] = get_or_set_cat(new_categories[i]);
	}

	for (const auto val : new_values) values.emplace_back(remap[val]);

	if (!contains_nav)
		contains_nav = std::any_of(new_values.begin(),
		    new_values.end(),
		    std::bind_front(std::equal_to{}, nav));
}
std::string_view data_source::dimension_t::get(
    std::size_t index) const
{
	return values[index] == nav ? std::string_view{}
	                            : categories[values[index]];
}

void data_source::dimension_t::set(std::size_t index,
    std::string_view value)
{
	values[index] =
	    value.data() != nullptr ? get_or_set_cat(value) : nav;
}
void data_source::dimension_t::set_nav(std::string_view value,
    std::size_t to_size)
{
	if (value.data() == nullptr) value = "";

	std::optional<std::uint32_t> cat_ix;
	if (values.size() < to_size)
		values.resize(to_size, *(cat_ix = get_or_set_cat(value)));

	if (!contains_nav) return;

	if (!cat_ix) cat_ix = get_or_set_cat(value);

	for (auto &val : values)
		if (val == nav) val = *cat_ix;

	contains_nav = false;
}

std::vector<bool>
data_source::dimension_t::get_categories_usage() const
{
	std::vector<bool> used(categories.size());
	for (const auto &val : values)
		if (val != nav) used[val] = true;

	return used;
}

void data_source::dimension_t::remove_unused_categories(
    std::vector<bool> &&used)
{
	std::vector<std::uint32_t> remap(categories.size());
	std::size_t new_size{};
	for (std::size_t i{}; i < categories.size(); ++i)
		if (used[i])
			categories[remap[i] = new_size++] =
			    std::move(categories[i]);
	categories.resize(new_size);

	for (auto &val : values)
		if (val != nav) val = remap[val];
}

std::uint32_t data_source::dimension_t::get_or_set_cat(
    std::string_view cat)
{
	if (cat.data() == nullptr) return nav;
	auto it = std::find(categories.begin(), categories.end(), cat);
	if (it == categories.end())
		it = categories.emplace(categories.end(), cat);
	return static_cast<std::uint32_t>(it - categories.begin());
}

const double &data_source::measure_t::get(std::size_t index) const
{
	return index < values.size() ? values[index] : nan;
}

std::pair<double, double> data_source::measure_t::get_min_max() const
{
	auto mini = std::numeric_limits<double>::max();
	auto maxi = std::numeric_limits<double>::lowest();

	for (auto val : values) {
		if (std::isinf(val) || std::isnan(val)) continue;
		if (val < mini) mini = val;
		if (val > maxi) maxi = val;
	}
	return {mini, maxi};
}

std::pair<std::string, bool>
data_source::aggregating_type::add_aggregated(
    const_series_data &&data,
    const aggregator_type &aggregator)
{
	std::string name;
	switch (data) {
		using enum series_type;
	case dimension: name = unsafe_get<dimension>(data).first; break;
	case measure: name = unsafe_get<measure>(data).first; break;
	default: break;
	}

	if (aggregator != aggregator_type::sum)
		name = std::string{Refl::enum_name(aggregator)} + '(' + name
		     + ')';

	auto &&[it, succ] = meas.try_emplace(std::move(name),
	    std::move(data),
	    aggregators[aggregator]);
	return {it->first, succ};
}

}