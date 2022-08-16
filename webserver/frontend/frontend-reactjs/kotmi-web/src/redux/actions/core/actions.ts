import * as types from './constants';
import { action } from 'typesafe-actions';

export const authCompleted = (response: any) => action(types.AUTH, response);
export const authStatusLoaded = (response: any) => action(types.AUTH_STATUS, response);
export const onLogout = (response: any) => action(types.ON_LOGOUT, response);
export const menuLoaded = (response: any) => action(types.GET_MENU, response);
export const moduleCreated = (response: any) => action(types.CREATE_MODULE, response);
