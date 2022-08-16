import { combineReducers } from 'redux';
import licenseReducer from './license';

const rootReducer = combineReducers({
    license: licenseReducer,
});

export type AppState = ReturnType<typeof rootReducer>;

export default rootReducer;
