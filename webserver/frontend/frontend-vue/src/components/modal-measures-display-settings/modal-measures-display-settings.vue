<template>
  <div class="modal-measures-display-settings">
    <a-modal
      :visible="isOpened"
      :mask="false"
      title="Просмотр сигналов"
      @cancel="closeModal"
    >
      <template #footer>
        <a-button
          key="ok"
          type="primary"
          @click="closeModal"
        >
          Ok
        </a-button>
      </template>

      <template #default>
        <template v-for="chart in chartMeasures">
          <h4 :key="`${chart.chartName}-h3`">
            {{ chart.chartName }}
          </h4>

          <a-list
            :key="`${chart.chartName}-list`"
            :data-source="chart.measures"
            size="small"
          >
            <template #renderItem="{ item }">
              <a-list-item>
                <div class="modal-measures-display-settings-list-item">
                  <div class="modal-measures-display-settings-list-item__checkbox">
                    <a-checkbox
                      :checked="item.isVisible"
                      @change="updateMeasureVisibleStatus($event, item.id)"
                    />
                  </div>

                  <div class="modal-measures-display-settings-list-item__color">
                    <a-tag :color="item.color">
                      &nbsp;
                    </a-tag>
                  </div>

                  <div class="modal-measures-display-settings-list-item__id">
                    {{ item.id }}
                  </div>
                </div>
              </a-list-item>
            </template>
          </a-list>
        </template>
      </template>
    </a-modal>
  </div>
</template>

<script lang="ts">
import { useStore } from '../../services/store-service';
import { IRetrospectiveMeasure } from '../../entities/retrospective-measure';
import { assert } from '../../utils';

type TMeasuresList = {
  chartName: string,
  measures: IRetrospectiveMeasure[]
}[]

export default {
  props: {
    isOpened: {
      type: Boolean,
      default: false
    }
  },
  emits: {
    close: () => true,
    showMeasure: (_measureId: string) => true,
    hideMeasure: (_measureId: string) => true
  },

  setup (_, { emit }) {
    const closeModal = () => emit('close');
    const store = useStore();
    const chartMeasures: TMeasuresList = store.state.retrospective.charts.map((chart) => {
      const measures = chart.measureIds.map((measureId) => {
        const measure = store.state.retrospective.measures.find(item => item.id === measureId);
        assert(measure !== undefined, 'measure is undefined');
        return measure;
      });

      return {
        chartName: chart.name,
        measures
      };
    });

    const updateMeasureVisibleStatus = (checked: Event, measureId) => {
      if (checked.target.checked) {
        emit('showMeasure', measureId);
      } else {
        emit('hideMeasure', measureId);
      }
    };

    return {
      closeModal,
      updateMeasureVisibleStatus,
      chartMeasures
    };
  }
};
</script>

<style lang="less">
.modal-measures-display-settings-list-item {
  display: flex;

  &__color {
    margin-left: 10px;
  }

  &__id {
    margin-left: 10px;
  }
}
</style>
