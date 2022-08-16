<template>
  <div
    ref="elRef"
    class="vue3-resize-observer"
    tabindex="-1"
  />
</template>

<script>
import { onMounted, ref, onBeforeUnmount } from 'vue';
import { getInternetExplorerVersion } from '../../utils/compatibility';

let isIE;

function initCompat () {
  if (!initCompat.init) {
    initCompat.init = true;
    isIE = getInternetExplorerVersion() !== -1;
  }
}

export default {
  name: 'ResizeObserver',

  props: {
    showTrigger: {
      type: Boolean,
      default: false
    }
  },

  emits: ['notify'],

  setup (props, { emit }) {
    let _w = 0;
    let _h = 0;
    const elRef = ref(null);
    let _resizeObject = null;

    const compareAndNotify = () => {
      if (_w !== elRef.value.offsetWidth || _h !== elRef.value.offsetHeight) {
        _w = elRef.value.offsetWidth;
        _h = elRef.value.offsetHeight;

        emit('notify', {
          width: _w,
          height: _h
        });
      }
    };

    const addResizeHandlers = () => {
      _resizeObject.contentDocument.defaultView.addEventListener('resize', compareAndNotify);
      compareAndNotify();
    };

    const removeResizeHandlers = () => {
      if (_resizeObject && _resizeObject.onload) {
        if (!isIE && _resizeObject.contentDocument) {
          _resizeObject.contentDocument.defaultView.removeEventListener('resize', compareAndNotify);
        }

        elRef.value.removeChild(_resizeObject);

        _resizeObject.onload = null;
        _resizeObject = null;
      }
    };

    onMounted(() => {
      initCompat();

      _w = elRef.value.offsetWidth;
      _h = elRef.value.offsetHeight;

      const object = document.createElement('object');
      _resizeObject = object;

      object.setAttribute('aria-hidden', 'true');
      object.setAttribute('tabindex', '-1');
      object.onload = addResizeHandlers;
      object.type = 'text/html';

      if (isIE) {
        elRef.value.appendChild(object);
      }

      object.data = 'about:blank';

      if (!isIE) {
        elRef.value.appendChild(object);
      }

      if (props.showTrigger) {
        compareAndNotify();
      }
    });

    onBeforeUnmount(() => {
      removeResizeHandlers();
    });

    return { elRef };
  }
};
</script>

<style>
.vue3-resize-observer {
  position: absolute;
  top: 0;
  left: 0;
  z-index: -1;
  width: 100%;
  height: 100%;
  border: none;
  background-color: transparent;
  pointer-events: none;
  display: block;
  overflow: hidden;
  opacity: 0;
}

.vue3-resize-observer object {
  display: block;
  position: absolute;
  top: 0;
  left: 0;
  height: 100%;
  width: 100%;
  overflow: hidden;
  pointer-events: none;
  z-index: -1;
}
</style>
