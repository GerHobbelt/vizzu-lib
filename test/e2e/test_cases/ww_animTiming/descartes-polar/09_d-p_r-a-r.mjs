import { data_14 } from '../../../test_data/chart_types_eu.mjs'

const testSteps = [
	(chart) =>
		chart.animate({
			data: data_14,
			config: {
				channels: {
					x: { set: ['Year'] },
					y: { set: ['Country', 'Value 2 (+)'] },
					color: { set: ['Country'] }
				},
				title: 'Stacked Column Chart',
				geometry: 'rectangle',
				legend: null
			}
		}),

	(chart) =>
		chart.animate(
			{
				config: {
					channels: {
						x: { set: ['Year'] },
						y: { set: ['Value 2 (+)', 'Country'] },
						color: { set: ['Country'] }
					},
					title: 'Radial Area Chart',
					geometry: 'area',
					coordSystem: 'polar'
				}
			},
			{
				duration: 2.5,
				easing: 'cubic-bezier(.39,0,.35,.99)',
				coordSystem: {
					delay: 0,
					duration: 1
				},
				geometry: {
					delay: 0.5,
					duration: 0.5
				},
				x: {
					delay: 0,
					duration: 0.5
				},
				y: {
					delay: 0.5,
					duration: 0.5
				}
			}
		),

	(chart) =>
		chart.animate(
			{
				config: {
					channels: {
						x: { set: ['Year'] },
						y: { set: ['Country', 'Value 2 (+)'] },
						color: { set: ['Country'] }
					},
					title: 'Stacked Column Chart',
					geometry: 'rectangle',
					coordSystem: 'cartesian',
					legend: null
				}
			},
			{
				geometry: {
					delay: 0.7,
					duration: 0.5
				},
				x: {
					delay: 1
				}
			}
		)
]

export default testSteps
