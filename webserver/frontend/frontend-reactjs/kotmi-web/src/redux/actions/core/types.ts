import * as actions from './actions';
import { ActionType } from 'typesafe-actions';

export type CoreAction = ActionType<typeof actions>;
