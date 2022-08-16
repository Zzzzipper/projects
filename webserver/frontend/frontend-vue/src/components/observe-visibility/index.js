import ObserveVisibility from './directives/observe-visibility';

const install = (app) => {
  app.use({
    // eslint-disable-next-line no-shadow
    install (app) {
      app.directive('observe-visibility', ObserveVisibility);
    }
  });
};

// Plugin
const plugin = {
  install
};

export {
  ObserveVisibility,
  install
};

export default plugin;
