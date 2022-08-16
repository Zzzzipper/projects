<template>
  <div class="page-retrospective-table">
    <RecycleScroller
      class="scroller"
      :items="table"
      :item-size="32"
      key-field="date"
    >
      <!--      <template #before>-->
      <!--        <div>-->
      <!--          <div>Время</div>-->
      <!--          <div>1</div>-->
      <!--          <div>2</div>-->
      <!--          <div>3</div>-->
      <!--        </div>-->
      <!--      </template>-->

      <template #default="{ item }">
        <div class="cel">
          {{ convertTimestampToHumanReadableDate(item.date) }}
        </div>

        <template v-for="(value, index) in item.values">
          <div :key="`${item.date}-${value}-${index}`" class="cel">
            {{ value }}
          </div>
        </template>
      </template>
    </RecycleScroller>
  </div>
</template>

<script lang="ts">
import { computed, defineComponent } from 'vue';
// @todo: add typings
import { RecycleScroller } from '../../components/virtual-scroller';
import '../../components/virtual-scroller/vue3-virtual-scroller.css';
import { useStore } from '../../services/store-service';

export default defineComponent({
  name: 'VisualizationTable',

  components: {
    RecycleScroller
  },

  setup () {
    const store = useStore();

    const table = computed(() => {
      const convertedMeasures = store.state.retrospective.measures.map(measure =>
        Object.fromEntries(measure.values.map(item => [`${item.date.getTime()}`, item.value]))
      );

      const result = Object.entries(convertedMeasures.shift() as Record<string, number>);

      result.forEach((resultItem) => {
        convertedMeasures.forEach((convertedMeasure) => {
          resultItem.push(convertedMeasure[resultItem[0]]);
        });
      });

      const result2 = result.map((item) => {
        const [date, ...rest] = item;

        return {
          date,
          values: rest
        };
      });

      return result2;
    });

    const convertTimestampToHumanReadableDate = (timestamp: string) => {
      const date = (new Date(parseInt(timestamp)));
      return `${date.toLocaleDateString('ru-RU')} ${date.toLocaleTimeString('ru-RU')}`;
    };

    return { table, convertTimestampToHumanReadableDate };
  }
});
</script>

<style>
.page-retrospective-table {
  flex-grow: 1;
  display: flex;
  height: 100%;
  flex-direction: column;
}

.scroller {
  flex-grow: 1;
  overflow-y: auto;
  display: flex;
  height: 100%;
}

/*.vue-recycle-scroller__before {*/
/*  display: table-header-group;*/
/*}*/

/*.vue-recycle-scroller__before > div {*/
/*  display: table-row;*/
/*}*/

/*.vue-recycle-scroller__before > div > div {*/
/*  display: table-cell;*/
/*}*/

/*.vue-recycle-scroller__after {*/
/*  display: table-footer-group;*/
/*}*/

/*.vue-recycle-scroller__after > div {*/
/*  display: table-row;*/
/*}*/

/*.vue-recycle-scroller__after > div > div {*/
/*  display: table-cell;*/
/*}*/

.vue-recycle-scroller__item-wrapper {
  display: table;
}

.vue-recycle-scroller__item-wrapper > div {
  display: table-row;
  border-bottom: 1px solid #000;
  width: auto !important;
  height: 32px;
}

.vue-recycle-scroller__item-wrapper > div:first-child {
  border-top: 1px solid #000;
}

.cel {
  padding: 0 12px;
  height: 32px;
  vertical-align: middle;
  min-width: 150px;
  display: table-cell;
  border-right: 1px solid #000;
}

.cel:first-child {
  border-left: 1px solid #000;
}
</style>
