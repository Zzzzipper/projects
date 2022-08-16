import { IRetrospectiveChartService, RetrospectiveChartService } from './retrospective-chart-service';

declare const self: DedicatedWorkerGlobalScope;

export enum ERetrospectiveChartWorkerSignals {
  SET_CANVAS = 'set-canvas',
  HIDE_POINTS = 'hide-points',
  SHOW_POINTS = 'show-points',
  ADD_LINE = 'add-line',
  CLEAR_LINE = 'clear-line',
  ADD_POINT = 'add-point',
  SET_POINTS = 'set-points',
  SHIFT_POINT = 'shift-point',
  ENABLE_CROSSHAIR = 'enable-crosshair',
  DISABLE_CROSSHAIR = 'disable-crosshair',
  SET_CROSSHAIR_COORDINATES = 'set-crosshair-coordinates',
  SHOW_LINE = 'show-line',
  HIDE_LINE = 'hide-line',
  RENDER = 'render',
  DESTROY = 'destroy'
}

export const WorkerToken = 'kotmi-retrospective-chart-worker';

const retrospectiveChartService: IRetrospectiveChartService = new RetrospectiveChartService();

self.onmessage = async (message: MessageEvent<unknown>) => {
  /**
     * Браузерные расширения могут стучаться в этот воркер и вызывать ошибки если отсутствует проверка
     * входящих данных. В качестве защиты добавлен `WorkerToken` который проверяется на каждом сообщении.
     */
  if (!Array.isArray(message.data) || message.data[0] !== WorkerToken) {
    return;
  }

  const [, signal, options] = message.data as [string, ERetrospectiveChartWorkerSignals, unknown[]];

  switch (signal) {
    case ERetrospectiveChartWorkerSignals.SET_CANVAS:
      await retrospectiveChartService.setCanvas(
        ...(options as Parameters<IRetrospectiveChartService['setCanvas']>)
      );
      break;
    case ERetrospectiveChartWorkerSignals.HIDE_POINTS:
      await retrospectiveChartService.hidePoints();
      break;
    case ERetrospectiveChartWorkerSignals.SHOW_POINTS:
      await retrospectiveChartService.showPoints();
      break;
    case ERetrospectiveChartWorkerSignals.ADD_LINE:
      await retrospectiveChartService.addLine(
        ...(options as Parameters<IRetrospectiveChartService['addLine']>)
      );
      break;
    case ERetrospectiveChartWorkerSignals.CLEAR_LINE:
      await retrospectiveChartService.clearLine(
        ...(options as Parameters<IRetrospectiveChartService['clearLine']>)
      );
      break;
    case ERetrospectiveChartWorkerSignals.ADD_POINT:
      await retrospectiveChartService.addPoint(
        ...(options as Parameters<IRetrospectiveChartService['addPoint']>)
      );
      break;
    case ERetrospectiveChartWorkerSignals.SET_POINTS:
      await retrospectiveChartService.setPoints(
        ...(options as Parameters<IRetrospectiveChartService['setPoints']>)
      );
      break;
    case ERetrospectiveChartWorkerSignals.SHIFT_POINT:
      await retrospectiveChartService.shiftPoint(
        ...(options as Parameters<IRetrospectiveChartService['shiftPoint']>)
      );
      break;
    case ERetrospectiveChartWorkerSignals.ENABLE_CROSSHAIR:
      await retrospectiveChartService.enableCrosshair();
      break;
    case ERetrospectiveChartWorkerSignals.DISABLE_CROSSHAIR:
      await retrospectiveChartService.disableCrosshair();
      break;
    case ERetrospectiveChartWorkerSignals.SET_CROSSHAIR_COORDINATES:
      await retrospectiveChartService.setCrosshairCoordinates(
        ...(options as Parameters<IRetrospectiveChartService['setCrosshairCoordinates']>)
      );
      break;
    case ERetrospectiveChartWorkerSignals.SHOW_LINE:
      await retrospectiveChartService.showLine(
        ...(options as Parameters<IRetrospectiveChartService['showLine']>)
      );
      break;
    case ERetrospectiveChartWorkerSignals.HIDE_LINE:
      await retrospectiveChartService.hideLine(
        ...(options as Parameters<IRetrospectiveChartService['hideLine']>)
      );
      break;
    case ERetrospectiveChartWorkerSignals.RENDER:
      await retrospectiveChartService.render();
      break;
    case ERetrospectiveChartWorkerSignals.DESTROY:
      await retrospectiveChartService.destroy();
      break;
    default:
      throw new TypeError('unknown worker signal');
  }
  self.postMessage([]);
};
