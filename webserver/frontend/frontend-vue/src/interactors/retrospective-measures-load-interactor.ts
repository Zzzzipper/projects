import { toRaw } from 'vue';
import BackendApiService from '../services/backend-api-service';
import config from '../config';
import { useStore } from '../services/store-service';
import { assert } from '../utils';

export class RetrospectiveMeasuresLoadInteractor {
  public static async execute (startTime: number, endTime: number) {
    const backendApiService = new BackendApiService(config.backendApiUrl);
    const store = useStore();
    const charts = store.state.retrospective.charts;

    await Promise.all(charts.map(async (chart) => {
      await Promise.all(chart.measureIds.map(async (measureId) => {
        const measure = store.state.retrospective.measures.find(item => item.isVisible && item.id === measureId);
        assert(measure !== undefined, 'measure is undefined');

        const values = await backendApiService.getRetrospectiveMeasures(measure.id, startTime, endTime);
        store.pushRetrospectiveMeasureValues(measure.id, values);

        await chart.broker?.addLine(toRaw(measure), toRaw(values));
      }));
      await chart.broker?.render();
    }));
  }
}
