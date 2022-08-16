import { toRaw } from 'vue';
import { useStore } from '../services/store-service';
import { assert } from '../utils';
import BackendApiService from '../services/backend-api-service';
import config from '../config';

export class RetrospectiveMeasuresReloadInteractor {
  public static async execute (startTime: number, endTime: number) {
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
  }
}
