#ifndef CHANNELS_H
#define CHANNELS_H

#include <array>
#include <functional>
#include <vector>

#include "data/datacube/datacubeoptions.h"

#include "channel.h"

namespace Vizzu::Gen
{

class Channels
{
public:
	struct Pos
	{
		ChannelId channelId;
		int position;
	};

	[[nodiscard]] bool anyAxisSet() const;
	[[nodiscard]] bool isEmpty() const;

	[[nodiscard]] Data::DataCubeOptions::IndexSet
	getDimensions() const;
	[[nodiscard]] Data::DataCubeOptions::IndexSet getMeasures() const;
	[[nodiscard]] Data::DataCubeOptions::IndexSet getDimensions(
	    const std::vector<ChannelId> &channelTypes) const;
	[[nodiscard]] Data::DataCubeOptions getDataCubeOptions() const;

	[[nodiscard]] const Channel &at(const ChannelId &id) const;
	Channel &at(const ChannelId &id);

	std::pair<bool, Channel::OptionalIndex>
	addSeries(const ChannelId &id, const Data::SeriesIndex &index);
	void removeSeries(const Data::SeriesIndex &index);
	void removeSeries(const ChannelId &id,
	    const Data::SeriesIndex &index);
	bool clearSeries(const ChannelId &id);

	[[nodiscard]] bool isSeriesUsed(
	    const Data::SeriesIndex &index) const;

	void reset();

	bool operator==(const Channels &other) const;

	[[nodiscard]] Channels shadow() const;

private:
	Refl::EnumArray<ChannelId, Channel> channels =
	    []<std::size_t... Ix>(std::index_sequence<Ix...>)
	{
		return decltype(channels){
		    Channel::makeChannel(static_cast<ChannelId>(Ix))...};
	}(std::make_index_sequence<
	    std::tuple_size_v<decltype(channels)::base_array>>{});
};

}

#endif
