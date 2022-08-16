import { Chart, ChartDataset, Plugin, ScatterDataPoint } from 'chart.js';
import { assertIsNotUndefined } from '../utils';

// @todo: поправить отрисовку crosshair по дате (сейчас по координатам)
export class ChartjsPluginCrosshair {
  private isCrosshairEnabled = false;
  private crosshairXCoordinate = 0;

  enableCrosshair (chart: Chart) {
    this.isCrosshairEnabled = true;

    chart.notifyPlugins('afterEvent', {
      event: {
        type: 'mouseover',
        clientX: 0,
        clientY: 0
      }
    });
    chart.draw();
  }

  disableCrosshair (chart: Chart) {
    this.isCrosshairEnabled = false;

    chart.notifyPlugins('afterEvent', {
      event: {
        type: 'mouseout',
        clientX: 0,
        clientY: 0
      }
    });
    chart.draw();
  }

  setCoordinates (chart: Chart, x: number, y: number) {
    this.crosshairXCoordinate = x;

    chart.notifyPlugins('afterEvent', {
      event: {
        type: 'mousemove',
        clientX: x,
        clientY: y
      }
    });
    chart.draw();
  }

  get installer (): Plugin {
    return {
      id: 'crosshair',
      afterDraw: this.handleAfterDrawHook.bind(this)
    };
  }

  private handleAfterDrawHook (chart: Chart) {
    if (!this.isCrosshairEnabled) {
      return;
    }

    if (!this.isXCoordinateUnderChartLines(chart) || !this.isChartScalesInitialized(chart)) {
      return;
    }

    if (
      chart.data.datasets.length !== 0 &&
      chart.data.datasets.every(dataset => dataset.data.length !== 0)
    ) {
      this.drawCrosshair(chart);
      this.drawCrosshairPoints(chart);
    }
  }

  private isChartScalesInitialized (chart: Chart) {
    const xAxisId = chart.getDatasetMeta(0).xAxisID ?? 'x';
    const yAxisId = chart.getDatasetMeta(0).yAxisID ?? 'y';
    assertIsNotUndefined<string>(xAxisId);
    assertIsNotUndefined<string>(yAxisId);
    const xScale = chart.scales[xAxisId];
    const yScale = chart.scales[yAxisId];

    return xScale !== undefined && yScale !== undefined;
  }

  private isXCoordinateUnderChartLines (chart: Chart) {
    const xAxisId = chart.getDatasetMeta(0).xAxisID ?? 'x';
    assertIsNotUndefined<string>(xAxisId);
    const xScale = chart.scales[xAxisId];

    const leftLimit = xScale.getPixelForValue(xScale.min, 0);
    const rightLimit = xScale.getPixelForValue(xScale.max, 0);
    return this.crosshairXCoordinate > leftLimit && this.crosshairXCoordinate < rightLimit;
  }

  private drawCrosshair (chart: Chart) {
    const yAxisId = chart.getDatasetMeta(0).yAxisID ?? 'y';
    assertIsNotUndefined<string>(yAxisId);
    const yScale = chart.scales[yAxisId];

    if (yScale === undefined) {
      return;
    }

    chart.ctx.beginPath();
    chart.ctx.moveTo(this.crosshairXCoordinate, yScale.getPixelForValue(yScale.max, 0));
    chart.ctx.lineWidth = 2;
    chart.ctx.strokeStyle = '#000';
    chart.ctx.lineTo(this.crosshairXCoordinate, yScale.getPixelForValue(yScale.min, 0));
    chart.ctx.stroke();
  }

  private drawCrosshairPoints (chart: Chart) {
    for (const dataset of (chart.data.datasets as ChartDataset<'line', ScatterDataPoint[]>[])) {
      const datasetIndex = chart.data.datasets.indexOf(dataset);
      const meta = chart.getDatasetMeta(datasetIndex);
      const yScale = chart.scales[meta.yAxisID ?? 'y'];
      const xScale = chart.scales[meta.xAxisID ?? 'x'];
      const xValue = xScale.getValueForPixel(this.crosshairXCoordinate);
      assertIsNotUndefined<number>(xValue);

      chart.ctx.beginPath();
      chart.ctx.arc(
        this.crosshairXCoordinate,
        yScale.getPixelForValue(this.getInterpolatedYValueForX(xValue, dataset), datasetIndex),
        3,
        0,
        2 * Math.PI,
        false
      );
      chart.ctx.fillStyle = 'white';
      chart.ctx.lineWidth = 2;
      chart.ctx.strokeStyle = '#000';
      chart.ctx.fill();
      chart.ctx.stroke();
    }
  }

  private getInterpolatedYValueForX (x: number, dataset: ChartDataset<'line', ScatterDataPoint[]>): number {
    const index = dataset.data.findIndex(item => item.x >= x);
    const previousPoint = dataset.data[index - 1];
    const currentPoint = dataset.data[index];

    if (previousPoint !== undefined && currentPoint !== undefined) {
      const slope = (currentPoint.y - previousPoint.y) / (currentPoint.x - previousPoint.x);
      return previousPoint.y + (x - previousPoint.x) * slope;
    } else if (previousPoint === undefined && currentPoint !== undefined) {
      return currentPoint.y;
    } else {
      throw new TypeError('there is no points');
    }
  }
}
