import configStore from '../redux/configureStore';
import { Provider } from 'react-redux';
import { HashRouter } from 'react-router-dom';
import AppWrapper from './core/AppWrapper';
import Api from '../api/index';

export interface IThunkExtraParam {
  api: Api;
}

const extra = {
  api: new Api(''),
};

export default () => {
  return (
    <HashRouter>
      <Provider store={configStore({}, extra)}>
        <AppWrapper />
      </Provider>
    </HashRouter>
  )
};
