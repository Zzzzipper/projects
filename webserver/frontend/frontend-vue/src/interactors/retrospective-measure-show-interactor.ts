import { useStore } from '../services/store-service';
import { assert } from '../utils';

export class RetrospectiveMeasureShowInteractor {
  public static async execute (measureId: string) {
    const store = useStore();

    store.updateRetrospectiveMeasure(measureId, {
      isVisible: true
    });

    const chart = store.state.retrospective.charts.find(chart => chart.measureIds.includes(measureId));
    assert(chart !== undefined, 'chart is undefined');

    await chart.broker?.showLine(measureId);
    await chart.broker?.render();
  }
}
