import ResizeObserver from './package/ResizeObserver';

// Install the components
const install = (app) => {
  app.use(ResizeObserver);
};

// Plugin
const Vue3Resize = {
  install
};

export {
  ResizeObserver,
  install
};

export default Vue3Resize;
