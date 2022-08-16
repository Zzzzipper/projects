import ResizeObserver from './ResizeObserver.vue';

ResizeObserver.install = function (app) {
  app.component(ResizeObserver.name, ResizeObserver);
};

export default ResizeObserver;
