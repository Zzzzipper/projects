import { App } from 'vue';
import { createRouter, createWebHistory } from 'vue-router';

const PageIndexComponent = () => import('../pages/page-index/page-index.vue');
const PageRetrospectiveComponent = () => import('../pages/page-retrospective/page-retrospective.vue');
const PageAuthenticationComponent = () => import('../pages/page-authentication/page-authentication.vue');
const PageLicenseComponent = () => import('../pages/page-licenses/page-licenses.vue');

const routes = [
  {
    path: '/',
    component: PageIndexComponent,
  },
  {
    path: '/retrospective',
    component: PageRetrospectiveComponent,
  },
  {
    path: '/authentication',
    component: PageAuthenticationComponent,
  },
  {
    path: '/license',
    component: PageLicenseComponent,
  }];

const router = createRouter({
  history: createWebHistory(),
  routes
});

export function useRouter(app: App) {
  app.use(router);
}
