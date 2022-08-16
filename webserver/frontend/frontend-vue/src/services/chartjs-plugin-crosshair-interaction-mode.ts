import { Chart, Interaction } from 'chart.js';

// @ts-ignore
Interaction.modes['x-custom'] = function (chart: Chart, e: any) {
  const items: any = [];

  for (let datasetIndex = 0; datasetIndex < chart.data.datasets.length; datasetIndex++) {
    const meta = chart.getDatasetMeta(datasetIndex);
    // do not interpolate hidden charts
    if (meta.hidden) {
      continue;
    }

    // @ts-ignore
    const xScale = chart.scales[meta.xAxisID];
    // @ts-ignore
    const yScale = chart.scales[meta.yAxisID];

    if (xScale === undefined || yScale === undefined) {
      return [];
    }

    const xValue = xScale.getValueForPixel(e.clientX);
    const data = chart.data.datasets[datasetIndex].data;
    const index = data.findIndex(function (o: any) {
      return o.x >= xValue;
    });

    if (index === -1) {
      continue;
    }

    // linear interpolate value
    const prev = data[index - 1];
    const next = data[index];

    let interpolatedValue = 0;
    if (prev && next) {
      // @ts-ignore
      const slope = (next.y - prev.y) / (next.x - prev.x);
      // @ts-ignore
      interpolatedValue = prev.y + (xValue - prev.x) * slope;
    }

    if (isNaN(interpolatedValue)) {
      continue;
    }

    const yPosition = yScale.getPixelForValue(interpolatedValue);

    // do not interpolate values outside of the axis limits
    if (isNaN(yPosition)) {
      continue;
    }

    // create a 'fake' event point

    const fakePoint = {
      element: {
        value: interpolatedValue,
        xValue,
        tooltipPosition () {
          return this._model;
        },
        hasValue () {
          return true;
        },
        _model: {
          x: e.clientX,
          y: yPosition
        },
        _datasetIndex: datasetIndex,
        _index: items.length,
        _xScale: {
          getLabelForIndex (indx: any) {
            return items[indx].xValue;
          }
        },
        _yScale: {
          getLabelForIndex (indx: any) {
            return items[indx].value;
          }
        },
        _chart: chart
      },
      datasetIndex,
      index
    };

    items.push(fakePoint);
  }

  // add other, not interpolated, items
  // const xItems = Chart.Interaction.modes.x(chart, e, options);
  // for (index = 0; index < xItems.length; index++) {
  //   const item = xItems[index];
  //   if (!chart.data.datasets[item._datasetIndex].interpolate) {
  //     items.push(item);
  //   }
  // }

  return items;
};
