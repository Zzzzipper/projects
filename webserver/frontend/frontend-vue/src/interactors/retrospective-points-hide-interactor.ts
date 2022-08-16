import { useStore } from '../services/store-service';
import { assert } from '../utils';

export class RetrospectivePointsHideInteractor {
  public static async execute () {
    const store = useStore();
    const charts = store.state.retrospective.charts;

    await Promise.all(charts.map(async (chart) => {
      assert(chart.broker !== undefined, 'chart.broker is undefined');
      await chart.broker.hidePoints();
      await chart.broker.render();
    }));
  }
}