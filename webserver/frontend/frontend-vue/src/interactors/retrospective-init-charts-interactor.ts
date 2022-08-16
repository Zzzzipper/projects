import { useStore } from '../services/store-service';
import { RetrospectiveChartBroker } from '../services/retrospective-chart-broker';
import { assert } from '../utils';

export class RetrospectiveInitChartsInteractor {
  public static async execute (
    containerWidth: number,
    containerHeight: number,
    canvases: {[id: string]: HTMLCanvasElement}
  ) {
    const store = useStore();
    const charts = store.state.retrospective.charts;

    charts.forEach((chart, chartIndex) => {
      const chartBroker = new RetrospectiveChartBroker();
      const canvas = canvases[chart.id];
      assert(canvas !== undefined, 'canvas is undefined');

      const canvasWidth = containerWidth;
      const canvasHeight = Math.round(containerHeight / Object.keys(canvases).length);
      canvas.style.width = `${canvasWidth}px`;
      canvas.style.height = `${canvasHeight}px`;

      chartBroker.setCanvas(canvas.transferControlToOffscreen(), {
        width: canvasWidth,
        height: canvasHeight,
        devicePixelRatio: window.devicePixelRatio,
        isXTicksShown: chartIndex === charts.length - 1
      });

      chart.broker = chartBroker;
    });
  }
}
