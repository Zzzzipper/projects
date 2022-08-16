<template>
  <LayoutDefault>
    <div class="retrospective">
      <div class="retrospective__panel">
        <div class="retrospective-alert">
          <a-alert
            v-if="isRangeLimitExceeded"
            type="error"
            message="Максимальный диапазон времени выборки - 1 час"
            show-icon
          />
        </div>

        <div class="retrospective-panel">
          <div class="retrospective-panel__start-time">
            <ADatePicker
              v-model:value="settings.selectStartTime"
              placeholder="Начало выборки"
              :show-time="{ format: 'HH:mm:ss' }"
              :allow-clear="false"
              format="DD.MM.YYYY HH:mm:ss"
              @openChange="(isOpened) => !isOpened ? loadRetrospectiveSelection() : null"
            />
          </div>

          <div class="retrospective-panel__end-time">
            <ADatePicker
              v-model:value="settings.selectEndTime"
              :show-time="{ format: 'HH:mm:SS' }"
              :allow-clear="false"
              :disabled-date="(currentDate) => {
                return currentDate.isBefore(settings.selectStartTime)
              }"
              format="DD.MM.YYYY HH:mm:ss"
              placeholder="Конец выборки"
              @openChange="(isOpened) => !isOpened ? loadRetrospectiveSelection() : null"
            />
          </div>

          <div class="retrospective-panel__change-time-backward">
            <a-button
              type="default"
              @click="loadRetrospectiveSelectionBackward"
            >
              <left-outlined />
            </a-button>
          </div>

          <div class="retrospective-panel__change-time-forward">
            <a-button
              type="default"
              @click="loadRetrospectiveSelectionForward"
            >
              <right-outlined />
            </a-button>
          </div>

          <div class="retrospective-panel__reload-button">
            <a-button
              type="default"
              @click="syncRetrospectiveSelection"
            >
              <reload-outlined />
            </a-button>
          </div>

          <div class="retrospective-panel__autoreload-button">
            <a-button
              :type="settings.isAutoReloadEnabled ? 'primary' : 'default'"
              @click="toggleAutoReload"
            >
              <template #icon>
                <sync-outlined />
              </template>
              Автообновление
            </a-button>
          </div>

          <div class="retrospective-panel__visualization-type">
            <ASelect v-model:value="settings.visualizationType">
              <ASelectOption value="chart">
                График
              </ASelectOption>

              <ASelectOption value="table">
                Таблица
              </ASelectOption>
            </ASelect>
          </div>

          <div class="retrospective-panel__show-points-button">
            <a-button
              :type="settings.isLinePointsVisible ? 'primary' : 'default'"
              @click="togglePointsShow"
            >
              <template #icon>
                <dot-chart-outlined />
              </template>
              Показ точек
            </a-button>
          </div>

          <div v-if="isLoadingIndicatorShown" class="retrospective-panel__loading-indicator">
            <spinner />
          </div>

          <div class="retrospective-panel__toggle-measures-display">
            <a-button
              type="default"
              @click="openMeasuresDisplaySettingsModal"
            >
              <check-circle-outlined />
            </a-button>
          </div>
        </div>
      </div>

      <div class="retrospective__content">
        <template v-if="settings.visualizationType === 'chart'">
          <VisualizationChart
            @canvases-ready="initCanvases($event); loadMeasuresValues()"
            @crosshairs-enable="enableCrosshairs"
            @crosshairs-disable="disableCrosshairs"
            @crosshairs-set-coordinates="setCrosshairsCoordinates"
          />
        </template>

        <template v-else>
          <VisualizationTable />
        </template>
      </div>
    </div>

    <modal-measures-display-settings
      :is-opened="isModalMeasuresDisplaySettingsDisplayed"
      @show-measure="showMeasure"
      @hide-measure="hideMeasure"
      @close="closeMeasuresDisplaySettingsModal"
    />
  </LayoutDefault>
</template>

<script lang="ts">
import moment from 'moment';
import { defineComponent, ref, reactive, computed } from 'vue';
import { ReloadOutlined, DotChartOutlined, RightOutlined, LeftOutlined, SyncOutlined, CheckCircleOutlined } from '@ant-design/icons-vue';
import { useRoute } from 'vue-router';
import config from '../../config';
import { TRetrospectiveSettings } from '../../types';
import { Spinner } from '../../components/spinner';
import { LayoutDefault } from '../../layouts/layout-default';
import { RetrospectiveLoadConfigurationInteractor } from '../../interactors/retrospective-load-configuration-interactor';
import { RetrospectiveInitChartsInteractor } from '../../interactors/retrospective-init-charts-interactor';
import { RetrospectiveMeasuresLoadInteractor } from '../../interactors/retrospective-measures-load-interactor';
import { RetrospectivePointsHideInteractor } from '../../interactors/retrospective-points-hide-interactor';
import { RetrospectivePointsShowInteractor } from '../../interactors/retrospective-points-show-interactor';
import { RetrospectiveCrosshairsEnableInteractor } from '../../interactors/retrospective-crosshairs-enable-interactor';
import { RetrospectiveCrosshairsDisableInteractor } from '../../interactors/retrospective-crosshairs-disable-interactor';
import { RetrospectiveCrosshairsSetCoordinatesInteractor } from '../../interactors/retrospective-crosshairs-set-coordinates-interactor';
import { RetrospectiveMeasuresReloadInteractor } from '../../interactors/retrospective-measures-reload-interactor';
import { RetrospectiveMeasuresAutoReloadInteractor } from '../../interactors/retrospective-measures-auto-reload-interactor';
import { ModalMeasuresDisplaySettings } from '../../components/modal-measures-display-settings';
import { RetrospectiveMeasureShowInteractor } from '../../interactors/retrospective-measure-show-interactor';
import { RetrospectiveMeasureHideInteractor } from '../../interactors/retrospective-measure-hide-interactor';
import VisualizationChart from './visualizaiton-chart.vue';
import VisualizationTable from './visualization-table.vue';

export default defineComponent({
  name: 'PageRetrospective',

  components: {
    ReloadOutlined,
    DotChartOutlined,
    RightOutlined,
    LeftOutlined,
    SyncOutlined,
    CheckCircleOutlined,
    VisualizationTable,
    VisualizationChart,
    Spinner,
    LayoutDefault,
    ModalMeasuresDisplaySettings
  },

  props: {},

  async setup () {
    const route = useRoute();
    const retrospectiveFileId = route.query.fileId as string | null;
    if (retrospectiveFileId === null) {
      throw new Error('retrospectiveFileId is null');
    }
    await RetrospectiveLoadConfigurationInteractor.execute(retrospectiveFileId);

    const isLoadingIndicatorShown = ref(false);
    const currentTimeWithDelay = moment().subtract(config.retrospectiveMeasuresDisplayTimeDelayTime, 'milliseconds');
    const settings = reactive<TRetrospectiveSettings>({
      selectStartTime: currentTimeWithDelay.clone().subtract(1, 'hours'),
      selectEndTime: currentTimeWithDelay.clone(),
      isAutoReloadEnabled: false,
      isLinePointsVisible: false,
      visualizationType: 'chart'
    });
    const isRangeLimitExceeded = computed(() => (
      settings.selectEndTime.valueOf() - settings.selectStartTime.valueOf() > 60 * 60 * 1000
    ));

    const updateSelectionTimeBoundsOnAutoReload = (_: number, endTime: number) => {
      settings.selectEndTime = moment(endTime);
      settings.selectStartTime = settings.selectEndTime.clone().subtract(1, 'hour');
    };

    const showLoadingIndicatorOnSyncStarted = () => {
      isLoadingIndicatorShown.value = true;
    };

    const hideLoadingIndicatorOnSyncEnded = () => {
      isLoadingIndicatorShown.value = false;
    };

    const toggleAutoReload = async () => {
      if (settings.isAutoReloadEnabled) {
        settings.isAutoReloadEnabled = false;
        console.log('disable');
        await RetrospectiveMeasuresAutoReloadInteractor.disable();
      } else {
        settings.isAutoReloadEnabled = true;
        await RetrospectiveMeasuresAutoReloadInteractor.enable({
          onReloaded: updateSelectionTimeBoundsOnAutoReload,
          onSyncStarted: showLoadingIndicatorOnSyncStarted,
          onSyncEnded: hideLoadingIndicatorOnSyncEnded
        });
      }
    };

    const togglePointsShow = async () => {
      if (settings.isLinePointsVisible) {
        settings.isLinePointsVisible = false;
        await RetrospectivePointsHideInteractor.execute();
      } else {
        settings.isLinePointsVisible = true;
        await RetrospectivePointsShowInteractor.execute();
      }
    };

    const enableCrosshairs = async (chartId: string) => {
      await RetrospectiveCrosshairsEnableInteractor.execute(chartId);
    };

    const disableCrosshairs = async (chartId: string) => {
      await RetrospectiveCrosshairsDisableInteractor.execute(chartId);
    };

    const setCrosshairsCoordinates = async (payload: { x: number, y: number, chartId: string }) => {
      await RetrospectiveCrosshairsSetCoordinatesInteractor.execute(payload.chartId, payload.x, payload.y);
    };

    const modals = {
      isModalMeasuresDisplaySettingsDisplayed: ref(false)
    };

    return {
      isRangeLimitExceeded,
      isLoadingIndicatorShown,
      settings,
      toggleAutoReload,
      togglePointsShow,
      enableCrosshairs,
      disableCrosshairs,
      setCrosshairsCoordinates,
      ...modals
    };
  },

  methods: {
    async loadRetrospectiveSelection () {
      if (this.isRangeLimitExceeded) {
        return;
      }
      this.isLoadingIndicatorShown = true;

      await RetrospectiveMeasuresReloadInteractor.execute(
        this.settings.selectStartTime.valueOf(),
        this.settings.selectEndTime.valueOf()
      );
      this.isLoadingIndicatorShown = false;
    },

    async syncRetrospectiveSelection () {
      const selectionEndTime = moment().subtract(config.retrospectiveMeasuresDisplayTimeDelayTime);
      const selectionStartTime = selectionEndTime.clone().subtract(1, 'hour');

      this.isLoadingIndicatorShown = true;
      await RetrospectiveMeasuresReloadInteractor.execute(selectionStartTime.valueOf(), selectionEndTime.valueOf());
      this.isLoadingIndicatorShown = false;

      this.settings.selectStartTime = selectionStartTime;
      this.settings.selectEndTime = selectionEndTime;
    },

    async loadRetrospectiveSelectionForward () {
      this.isLoadingIndicatorShown = true;
      const selectionEndTime = this.settings.selectEndTime.clone().add(1, 'hour');
      const selectionStartTime = selectionEndTime.clone().subtract(1, 'hour');

      await RetrospectiveMeasuresReloadInteractor.execute(
        selectionStartTime.valueOf(),
        selectionEndTime.valueOf()
      );

      this.settings.selectEndTime = selectionEndTime;
      this.settings.selectStartTime = selectionStartTime;
      this.isLoadingIndicatorShown = false;
    },

    async loadRetrospectiveSelectionBackward () {
      this.isLoadingIndicatorShown = true;
      const selectionEndTime = this.settings.selectEndTime.clone().subtract(1, 'hour');
      const selectionStartTime = selectionEndTime.clone().subtract(1, 'hour');

      await RetrospectiveMeasuresReloadInteractor.execute(
        selectionStartTime.valueOf(),
        selectionEndTime.valueOf()
      );

      this.settings.selectEndTime = selectionEndTime;
      this.settings.selectStartTime = selectionStartTime;
      this.isLoadingIndicatorShown = false;
    },

    async initCanvases (payload: {
      containerWidth: number,
      containerHeight: number,
      canvases: { [id: string]: HTMLCanvasElement}
    }) {
      await RetrospectiveInitChartsInteractor.execute(
        payload.containerWidth,
        payload.containerHeight,
        payload.canvases
      );
    },

    async loadMeasuresValues () {
      await RetrospectiveMeasuresLoadInteractor.execute(
        this.settings.selectStartTime.valueOf(),
        this.settings.selectEndTime.valueOf()
      );
    },

    openMeasuresDisplaySettingsModal () {
      this.isModalMeasuresDisplaySettingsDisplayed = true;
    },

    closeMeasuresDisplaySettingsModal () {
      this.isModalMeasuresDisplaySettingsDisplayed = false;
    },

    async showMeasure (measureId: string) {
      await RetrospectiveMeasureShowInteractor.execute(measureId);
    },

    async hideMeasure (measureId: string) {
      await RetrospectiveMeasureHideInteractor.execute(measureId);
    }
  }
});
</script>

<style>
.retrospective {
  margin: 30px 30px 40px;
  flex-grow: 1;
  display: flex;
  flex-direction: column;
}

.retrospective__content {
  width: 100%;
  padding-top: 20px;
  flex-grow: 1;
  position: relative;
  display: flex;
  height: calc(100vh - 142px);
  flex-direction: column;
}

.retrospective__charts canvas {
  display: block;
}

.retrospective__panel {
  align-items: center;
}

.retrospective-panel {
  display: flex;
}

.retrospective-alert + .retrospective-panel {
  margin-top: 10px;
}

.retrospective-panel__start-time + .retrospective-panel__end-time {
  margin-left: 5px;
}

.retrospective-panel__change-time-backward {
  margin-left: 15px;
}

.retrospective-panel__change-time-forward {
  margin-left: 5px;
}

.retrospective-panel__reload-button {
  margin-left: 40px;
}

.retrospective-panel__autoreload-button {
  margin-left: 15px;
}

.retrospective-panel__show-points-button {
  margin-left: 15px;
}

.retrospective-panel__visualization-type {
  margin-left: 15px;
}

.retrospective-panel__loading-indicator {
  margin-left: 15px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.retrospective-panel__toggle-measures-display {
  margin-left: 15px;
}
</style>
