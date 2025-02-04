import { data } from '../../../../test_data/music_industry_history_1.mjs'

const description = `This is a 2-step animation:

1st: set the Split parameter to true

2nd:
- move the Measure to the X-axis
- set the Align parameter 'center'
- set the Split parameter to false
- arrange the markers in reverse order
- switch the Geometry from Area to Rectangle`
const testSteps = [
	(chart) => {
		chart.on('plot-axis-label-draw', (event) => {
			const year = parseFloat(event.detail.text)
			if (!event.detail.text.includes('$') && !isNaN(year) && year % 5 !== 0)
				event.preventDefault()
		})
		return chart
	},

	(chart) =>
		chart.animate({
			data,
			config: {
				channels: {
					x: 'Year',
					y: {
						set: ['Revenue', 'Format'],
						align: 'center'
					},
					color: 'Format'
				},

				geometry: 'area'
			},
			style: {
				plot: {
					xAxis: {
						label: {
							angle: 0
						}
					},
					yAxis: {
						label: {
							numberScale: 'K, M, B, T'
						}
					}
				}
			}
		}),

	(chart) =>
		chart.animate({
			y: {
				split: true
			}
		}),

	(chart) =>
		chart.animate({
			config: {
				channels: {
					y: { set: ['Revenue', 'Year'], align: 'none', split: false },
					x: {
						set: ['Format'],

						sort: 'byValue',
						reverse: true
					}
				},

				geometry: 'rectangle'
			},
			style: {
				plot: {
					xAxis: {
						label: {
							angle: null
						}
					}
				}
			}
		}),
	(chart) => {
		chart.feature('tooltip', true)
		return chart
	}
]

export { description }
export default testSteps
