import { toRaw } from 'vue';
import { useStore } from '../services/store-service';
import BackendApiService from '../services/backend-api-service';
import config from '../config';
import { assert } from '../utils';

type TAutoReloadCallbacks = {
  onReloaded?: (startTime: number, endTime: number) => void,
  onSyncStarted?: () => void,
  onSyncEnded?: () => void
}

export class RetrospectiveMeasuresAutoReloadInteractor {
  private static timeoutId: number = 0;

  private static lastUpdateEndTime: number = 0;

  private static callbacks: TAutoReloadCallbacks = {}

  public static async enable (autoReloadCallbacks: TAutoReloadCallbacks) {

    if (this.timeoutId != 0) return;

    const endTime = Date.now() - config.retrospectiveMeasuresDisplayTimeDelayTime;
    const startTime = endTime - (1000 * 60 * 60);
    Object.assign(this.callbacks, autoReloadCallbacks);

    if (this.callbacks.onSyncStarted !== undefined) {
      this.callbacks.onSyncStarted();
    }

    await this.synchronizeMeasuresValues(startTime, endTime);

    if (this.callbacks.onSyncEnded !== undefined) {
      this.callbacks.onSyncEnded();
    }

    if (this.callbacks.onReloaded !== undefined) {
      this.callbacks.onReloaded(startTime, endTime);
    }

    this.timeoutId = setInterval(this.updateMeasuresValues.bind(this), config.retrospectiveAutoReloadIntervalTime);
    console.log(this.timeoutId);
  }

  public static async disable () {
    console.log(this.timeoutId);
    clearInterval(this.timeoutId);
    this.callbacks = {};
    this.timeoutId = 0;
  }

  private static async updateMeasuresValues () {

    if (this.timeoutId == 0) return;

    const store = useStore();
    const backendApiService = new BackendApiService(config.backendApiUrl);
    const charts = store.state.retrospective.charts;
    const startTime = this.lastUpdateEndTime;
    const endTime = Date.now() - config.retrospectiveMeasuresDisplayTimeDelayTime;

    await Promise.all(charts.map(async (chart) => {
      await Promise.all(chart.measureIds.map(async (measureId) => {
        const measure = store.state.retrospective.measures.find(item => item.isVisible && item.id === measureId);
        assert(measure !== undefined, 'measure is undefined');

        const values = await backendApiService.getRetrospectiveMeasures(measure.id, startTime, endTime);

        store.pushRetrospectiveMeasureValues(measure.id, values);
        store.spliceFromStartRetrospectiveMeasureValues(measure.id, values.length);

        for (const value of values) {
          await chart.broker?.addPoint(toRaw(measure.id), toRaw(value));
          await chart.broker?.shiftPoint(toRaw(measure.id));
        }
      }));

      await chart.broker?.render();
    }));

    this.lastUpdateEndTime = endTime;
    if (this.callbacks.onReloaded !== undefined) {
      this.callbacks.onReloaded(startTime, endTime);
    }

    // this.timeoutId = setTimeout(this.updateMeasuresValues.bind(this), config.retrospectiveAutoReloadIntervalTime);
    console.log(this.timeoutId);
  }

  private static async synchronizeMeasuresValues (startTime: number, endTime: number) {
    const store = useStore();
    const backendApiService = new BackendApiService(config.backendApiUrl);
    const charts = store.state.retrospective.charts;

    await Promise.all(charts.map(async (chart) => {
      await Promise.all(chart.measureIds.map(async (measureId) => {
        const measure = store.state.retrospective.measures.find(item => item.isVisible && item.id === measureId);
        assert(measure !== undefined, 'measure is undefined');

        const values = await backendApiService.getRetrospectiveMeasures(measure.id, startTime, endTime);
        store.setRetrospectiveMeasureValues(measure.id, values);

        await chart.broker?.clearLine(toRaw(measure.id));
        await chart.broker?.setPoints(toRaw(measure.id), toRaw(values));
      }));

      await chart.broker?.render();
    }));

    this.lastUpdateEndTime = endTime;
  }
}
