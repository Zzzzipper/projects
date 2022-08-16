import { combineReducers } from 'redux';
import coreReducer from './core';
import retroReducer from './retrospective';

const rootReducer = combineReducers({
    core: coreReducer,
    retro: retroReducer,
});

export type AppState = ReturnType<typeof rootReducer>;

export default rootReducer;
