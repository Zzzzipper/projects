import { applyMiddleware, createStore, Store } from 'redux';
import logger from 'redux-logger';
import thunk from 'redux-thunk';
import rootReducer from './reducers/index';
import { AppState } from './reducers';
import { IThunkExtraParam } from '../app/App';

export default function configureStore(
    initialState = {},
    extra: IThunkExtraParam,
): Store<AppState> {
    const middleWares = [ thunk.withExtraArgument<IThunkExtraParam>(extra) ];
    return createStore(rootReducer, initialState, applyMiddleware(...middleWares, logger));
}
