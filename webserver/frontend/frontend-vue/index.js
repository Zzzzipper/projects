import { createApp } from 'vue';
import App from './app.vue';
import { useAnt } from './src/ant/ant';
import { useRouter } from './src/router/router';

const app = createApp(App);
useAnt(app);
useRouter(app);

app.mount('#app');

// Hot Module Replacement (HMR) - Remove this snippet to remove HMR.
// Learn more: https://www.snowpack.dev/concepts/hot-module-replacement
if (import.meta.hot) {
  import.meta.hot.accept();
  import.meta.hot.dispose(() => {
    app.unmount();
  });
}
