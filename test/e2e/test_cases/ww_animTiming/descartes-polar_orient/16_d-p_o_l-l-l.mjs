import { data_14 } from '../../../test_data/chart_types_eu.mjs'

const testSteps = [
	(chart) =>
		chart.animate({
			data: data_14,
			config: {
				channels: {
					x: { set: ['Year'] },
					y: { set: ['Value 2 (+)'] },
					color: { set: ['Country'] }
				},
				title: 'Line Chart',
				geometry: 'line',
				legend: null
			}
		}),

	(chart) =>
		chart.animate(
			{
				config: {
					channels: {
						x: { set: ['Value 2 (+)'] },
						noop: { set: ['Year'] },
						y: { set: ['Country'] },
						color: { set: ['Country'] }
					},
					title: 'Radial Line Chart Vertical',
					geometry: 'line',
					coordSystem: 'polar',
					orientation: 'vertical'
				}
			},
			{
				easing: 'cubic-bezier(0.65,0,0.65,1)',
				coordSystem: {
					delay: 0,
					duration: 1
					//                easing: 'linear'
				},
				geometry: {
					delay: 0,
					duration: 0
					//                easing: 'linear'
				},
				x: {
					delay: 0.25,
					duration: 0.75
					//               easing: 'ease-in'
				},
				y: {
					delay: 0,
					duration: 0.75
					//                easing: 'cubic-bezier(.39,0,.35,.99)'
				}
			}
		),

	(chart) =>
		chart.animate(
			{
				config: {
					channels: {
						x: { set: ['Year'] },
						y: { set: ['Value 2 (+)'] },
						color: { set: ['Country'] }
					},
					title: 'Line Chart',
					geometry: 'line',
					coordSystem: 'cartesian',
					orientation: 'horizontal',
					legend: null
				}
			},
			{
				easing: 'cubic-bezier(0.65,0,0.65,1)',
				coordSystem: {
					delay: 0,
					duration: 1
					//                easing: 'linear'
				},
				geometry: {
					delay: 0,
					duration: 0
					//                easing: 'linear'
				},
				x: {
					delay: 0,
					duration: 0.75
					//               easing: 'ease-in'
				},
				y: {
					delay: 0.25,
					duration: 0.75
					//                easing: 'cubic-bezier(.39,0,.35,.99)'
				}
			}
		)
]

export default testSteps
