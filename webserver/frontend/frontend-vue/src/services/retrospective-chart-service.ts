import {
  Chart,
  LinearScale,
  LineController,
  LineElement,
  PointElement,
  ScatterDataPoint,
  TimeScale,
  Title,
  Tooltip,
} from 'chart.js';
import { IRetrospectiveValue } from '../entities/retrospective-value';
import { IRetrospectiveMeasure } from '../entities/retrospective-measure';
import 'chartjs-adapter-moment';
import { assertIsNotUndefined } from '../utils';
import { ERetrospectiveMeasureStyle } from '../types';
import { ChartjsPluginCrosshair } from './chartjs-plugin-crosshair';
import './chartjs-plugin-crosshair-interaction-mode';

Chart.register(LineController, LineElement, PointElement, LinearScale, Title, TimeScale, Tooltip);

const POINT_RADIUS = 2;

export type TRetrospectiveChartServiceCanvasOptions = {
  width: number,
  height: number,
  devicePixelRatio: number,
  isXTicksShown: boolean,
}

export interface IRetrospectiveChartService {
  setCanvas(canvas: HTMLCanvasElement | OffscreenCanvas, options: TRetrospectiveChartServiceCanvasOptions): Promise<void>;
  hidePoints(): Promise<void>;
  showPoints(): Promise<void>;
  addLine(measureSettings: IRetrospectiveMeasure, measures: IRetrospectiveValue[]): Promise<void>;
  clearLine(measureId: string): Promise<void>;
  addPoint(measureId: string, measure: IRetrospectiveValue): Promise<void>;
  setPoints(measureId: string, measures: IRetrospectiveValue[]): Promise<void>;
  shiftPoint(measureId: string): Promise<void>;
  enableCrosshair(): Promise<void>;
  disableCrosshair(): Promise<void>;
  setCrosshairCoordinates(x: number, y: number): Promise<void>;
  showLine(measureId: string): Promise<void>;
  hideLine(measureId: string): Promise<void>;
  render(): Promise<void>;
  destroy(): Promise<void>;
}

export class RetrospectiveChartService implements IRetrospectiveChartService {
  private chart: Chart | undefined = undefined;
  private crosshairPlugin: ChartjsPluginCrosshair;
  private measureIdToLineIndexMap: { [id: string]: number } = {};

  constructor () {
    this.crosshairPlugin = new ChartjsPluginCrosshair();
  }

  async setCanvas (
    canvas: HTMLCanvasElement | OffscreenCanvas,
    options: TRetrospectiveChartServiceCanvasOptions
  ) {
    this.chart = this.initChartJs(canvas, options);
    this.chart.canvas.width = options.width;
    this.chart.canvas.height = options.height;
    this.chart.resize();
  }

  private initChartJs (
    canvas: HTMLCanvasElement | OffscreenCanvas,
    { devicePixelRatio, isXTicksShown }: TRetrospectiveChartServiceCanvasOptions
  ) {
    const context = canvas.getContext('2d');
    if (context === null) {
      throw new Error('canvas context is null');
    }

    return new Chart(context, {
      type: 'line',
      data: {
        labels: [],
        datasets: [],
      },
      plugins: [this.crosshairPlugin.installer],
      options: {
        interaction: {
          mode: ('x-custom' as 'x'),
        },
        spanGaps: true,
        devicePixelRatio,
        animation: false,
        events: [],
        elements: {
          point: {
            radius: 0,
          }
        },
        layout: {
          padding: 20,
        },
        responsive: false,
        maintainAspectRatio: false,
        plugins: {
          legend: {
            display: false,
          },
          tooltip: {
            enabled: true,
          }
        },
        scales: {
          x: {
            ticks: {
              display: isXTicksShown,
              maxRotation: 90,
              minRotation: 90,
            },
            type: 'time',
            time: {
              unit: 'minute',
              tooltipFormat: 'DD.MM.YYYY HH:mm',
              displayFormats: {
                minute: 'DD.MM.YYYY HH:mm',
              }
            }
          }
        }
      }
    });
  }

  async hidePoints () {
    if (this.chart?.config?.options?.elements?.point?.radius === undefined) {
      throw new TypeError('point radius is undefined');
    }

    this.chart.config.options.elements.point.radius = 0;
  }

  async showPoints () {
    if (this.chart?.config?.options?.elements?.point?.radius === undefined) {
      throw new TypeError('point radius is undefined');
    }

    this.chart.config.options.elements.point.radius = POINT_RADIUS;
  }

  async addLine (measure: IRetrospectiveMeasure, measureValues: IRetrospectiveValue[]) {
    assertIsNotUndefined<Chart>(this.chart);

    let borderDash: number[] = [];
    switch (measure.style) {
      case ERetrospectiveMeasureStyle.SOLID:
        borderDash = [];
        break;
      case ERetrospectiveMeasureStyle.DASHED_LONG_LINES:
        borderDash = [5, 2];
        break;
      case ERetrospectiveMeasureStyle.DASHED_SHORT_LINES:
        borderDash = [2, 2];
        break;
      case ERetrospectiveMeasureStyle.DASHED_VARIABLE_LENGTH_LINES:
        borderDash = [4, 2, 2, 2];
    }

    const length = this.chart.data.datasets.push({
      fill: false,
      showLine: true,
      parsing: false,
      tension: 1,
      borderDash,
      hidden: !measure.isVisible,
      borderWidth: measure.width,
      backgroundColor: measure.color,
      borderColor: measure.color,
      data: measureValues.map(item => ({
        x: item.date.getTime(),
        y: item.value,
      })),
    });

    this.measureIdToLineIndexMap[measure.id] = length - 1;
  }

  async clearLine (measureId: string) {
    assertIsNotUndefined<Chart>(this.chart);
    const lineIndex = this.measureIdToLineIndexMap[measureId];

    const line = this.chart.data.datasets[lineIndex];
    if (line === undefined) {
      throw new Error('line is undefined');
    }

    line.data = [];
  }

  async addPoint (measureId: string, measure: IRetrospectiveValue) {
    assertIsNotUndefined<Chart>(this.chart);
    const lineIndex = this.measureIdToLineIndexMap[measureId];

    const dataset = this.chart.data.datasets[lineIndex];
    assertIsNotUndefined(dataset);
    const data = dataset.data as ScatterDataPoint[];

    data.push({
      x: measure.date.getTime(),
      y: measure.value,
    });
  }

  async setPoints (measureId: string, measures: IRetrospectiveValue[]) {
    assertIsNotUndefined<Chart>(this.chart);
    const lineIndex = this.measureIdToLineIndexMap[measureId];

    const dataset = this.chart.data.datasets[lineIndex];
    assertIsNotUndefined(dataset);
    dataset.data = measures.map(item => ({
      x: item.date.getTime(),
      y: item.value,
    }));
  }

  async shiftPoint (measureId: string) {
    assertIsNotUndefined<Chart>(this.chart);
    const lineIndex = this.measureIdToLineIndexMap[measureId];
    this.chart.data.datasets[lineIndex].data.shift();
  }

  async enableCrosshair () {
    assertIsNotUndefined<Chart>(this.chart);
    this.crosshairPlugin.enableCrosshair(this.chart);
  }

  async disableCrosshair () {
    assertIsNotUndefined<Chart>(this.chart);
    this.crosshairPlugin.disableCrosshair(this.chart);
  }

  async setCrosshairCoordinates (x: number, y: number) {
    assertIsNotUndefined<Chart>(this.chart);
    this.crosshairPlugin.setCoordinates(this.chart, x, y);
  }

  async showLine (measureId: string) {
    assertIsNotUndefined<Chart>(this.chart);
    const lineIndex = this.measureIdToLineIndexMap[measureId];
    this.chart.data.datasets[lineIndex].hidden = false;
  }

  async hideLine (measureId: string) {
    assertIsNotUndefined<Chart>(this.chart);
    const lineIndex = this.measureIdToLineIndexMap[measureId];
    this.chart.data.datasets[lineIndex].hidden = true;
  }

  async render () {
    assertIsNotUndefined<Chart>(this.chart);
    this.chart.update();
  }

  async destroy () {
    assertIsNotUndefined<Chart>(this.chart);
    this.chart.destroy();
  }
}
