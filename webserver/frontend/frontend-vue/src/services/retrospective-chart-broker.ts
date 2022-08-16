import { IRetrospectiveMeasure } from '../entities/retrospective-measure';
import { IRetrospectiveValue } from '../entities/retrospective-value';
import { IRetrospectiveChartService, TRetrospectiveChartServiceCanvasOptions } from './retrospective-chart-service';
import { ERetrospectiveChartWorkerSignals, WorkerToken } from './retrospective-chart-worker';

const WORKER_REQUEST_TIMEOUT = 5000;

export class RetrospectiveChartBroker implements IRetrospectiveChartService {
  private worker: Worker;

  constructor () {
    this.worker = new Worker(new URL('./retrospective-chart-worker.js', import.meta.url), {
      type: 'module',
      name: 'retrospective-chart-renderer'
    });
  }

  async setCanvas (canvas: OffscreenCanvas, options: TRetrospectiveChartServiceCanvasOptions) {
    await this.requestWorker<Parameters<IRetrospectiveChartService['setCanvas']>>(
      [ERetrospectiveChartWorkerSignals.SET_CANVAS, [canvas, options]],
      [canvas]
    );
  }

  async addLine (measureSettings: IRetrospectiveMeasure, measures: IRetrospectiveValue[]) {
    await this.requestWorker(
      [ERetrospectiveChartWorkerSignals.ADD_LINE, [measureSettings, measures]]
    );
  }

  async addPoint (measureId: string, measure: IRetrospectiveValue) {
    await this.requestWorker<Parameters<IRetrospectiveChartService['addPoint']>>(
      [ERetrospectiveChartWorkerSignals.ADD_POINT, [measureId, measure]]
    );
  }

  async setPoints (measureId: string, measures: IRetrospectiveValue[]) {
    await this.requestWorker<Parameters<IRetrospectiveChartService['setPoints']>>(
      [ERetrospectiveChartWorkerSignals.SET_POINTS, [measureId, measures]]
    );
  }

  async clearLine (measureId: string) {
    await this.requestWorker<Parameters<IRetrospectiveChartService['clearLine']>>(
      [ERetrospectiveChartWorkerSignals.CLEAR_LINE, [measureId]]
    );
  }

  async hidePoints () {
    await this.requestWorker<Parameters<IRetrospectiveChartService['hidePoints']>>(
      [ERetrospectiveChartWorkerSignals.HIDE_POINTS]
    );
  }

  async enableCrosshair () {
    await this.requestWorker<Parameters<IRetrospectiveChartService['enableCrosshair']>>(
      [ERetrospectiveChartWorkerSignals.ENABLE_CROSSHAIR]
    );
  }

  async disableCrosshair () {
    await this.requestWorker<Parameters<IRetrospectiveChartService['disableCrosshair']>>(
      [ERetrospectiveChartWorkerSignals.DISABLE_CROSSHAIR]
    );
  }

  async setCrosshairCoordinates (x: number, y: number) {
    await this.requestWorker<Parameters<IRetrospectiveChartService['setCrosshairCoordinates']>>(
      [ERetrospectiveChartWorkerSignals.SET_CROSSHAIR_COORDINATES, [x, y]]
    );
  }

  async showLine (measureId: string) {
    await this.requestWorker<Parameters<IRetrospectiveChartService['showLine']>>(
      [ERetrospectiveChartWorkerSignals.SHOW_LINE, [measureId]]
    );
  }

  async hideLine (measureId: string) {
    await this.requestWorker<Parameters<IRetrospectiveChartService['hideLine']>>(
      [ERetrospectiveChartWorkerSignals.HIDE_LINE, [measureId]]
    );
  }

  async render () {
    await this.requestWorker<Parameters<IRetrospectiveChartService['render']>>(
      [ERetrospectiveChartWorkerSignals.RENDER]
    );
  }

  async shiftPoint (measureId: string) {
    await this.requestWorker<Parameters<IRetrospectiveChartService['shiftPoint']>>(
      [ERetrospectiveChartWorkerSignals.SHIFT_POINT, [measureId]]
    );
  }

  async showPoints () {
    await this.requestWorker<Parameters<IRetrospectiveChartService['showPoints']>>(
      [ERetrospectiveChartWorkerSignals.SHOW_POINTS]
    );
  }

  async destroy () {
    await this.requestWorker<Parameters<IRetrospectiveChartService['destroy']>>(
      [ERetrospectiveChartWorkerSignals.DESTROY]
    );
    this.worker.terminate();
  }

  private async requestWorker<T> (options: [ERetrospectiveChartWorkerSignals, T?], transfer?: Transferable[]) {
    return await new Promise((resolve, reject) => {
      const timeoutHandler = () => {
        reject(new Error('worker request timeout is excided'));
        this.worker.removeEventListener('message', messageHandler);
      };
      const timeoutId = setTimeout(timeoutHandler, WORKER_REQUEST_TIMEOUT);

      function messageHandler (message: MessageEvent) {
        resolve(message.data);
        clearTimeout(timeoutId);
      };

      this.worker.addEventListener('message', messageHandler, {
        once: true
      });

      if (transfer === undefined) {
        this.worker.postMessage([WorkerToken, ...options]);
      } else {
        this.worker.postMessage([WorkerToken, ...options], transfer);
      }
    });
  }
}
