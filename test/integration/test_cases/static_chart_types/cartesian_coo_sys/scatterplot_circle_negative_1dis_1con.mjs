import { data } from '/test/integration/test_data/chart_types_eu.js';

const testSteps = [
    chart => chart.animate(
        {
            data: data,
            config:
            {
                channels:
                {
                    x: { attach: ['Year'] },
                    y: { attach: ['Value 5 (+/-)'], range: { min: '0%', max: '110%' } },
                    label: { attach: ['Value 5 (+/-)'] }
                },
                title: 'Scatterplot',
                align: 'none',
                geometry: 'circle'
            }
        }
    )
];

export default testSteps;