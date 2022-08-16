import { Moment } from 'moment';
import { ILicense } from './entities/licenses';

type TBackendRetrospectiveSettingsMeas = {
  '-align': 'Default',
  '-color': string, // Цвет линии - number
  '-formatvalue': '2', // Кол-во знаков после запятой у значения - number
  '-id': string, // Id сигнала - string
  '-linestep': 'False',
  '-pointer': 'False',
  '-presentationtype': '1',
  '-shift': '+0.0.0 0:0:0',
  '-style': string // Стиль линии - number,
  '-visible': string, // Показана ли линия - boolean
  '-width': '1',
}

type TBackendRetrospecitveSettingsChart = {
  '-gradientstartcolor': '16777215',
  '-name': string // Имя графика - string
  '-charttype': string, // Тип графика - number
  '-backgroundcolor': '16777215',
  '-limitautomatic': 'True',
  '-view3d': 'False',
  '-gradientdirection': '0',
  '-limitminimum': '0',
  '-legendcolor': '16777215',
  '-limitmaximum': '0',
  '-gradientvisible': 'False',
  '-gradientendcolor': '16777215',
  '-legendvisible': 'True',
  '-legendalignment': '2'
  'meas': TBackendRetrospectiveSettingsMeas[] | TBackendRetrospectiveSettingsMeas,
}

export type TBackendResponseRetrospectiveSettings = {
  'retro': {
    '-datamode': '0',
    '-interval': '0.0.0 0:10:0',
    '-step': '0.0.0 0:1:0',
    '-caption': 'Приречная ГРЭС - Генерация', // Описание - string
    '-tableorientation': '0',
    '-timeorientation': '0',
    '-formattime': ' hh:nn:ss.zzz',
    '-align': 'None',
    'chart': TBackendRetrospecitveSettingsChart | TBackendRetrospecitveSettingsChart[],
  },
};

export type TBackendResponseRetrospectiveMeasures = {
  'DT': number, // unix timestamp снятия значения
  'DTCP': number,
  'FLG': number,
  'ID': number,
  'VAL': number, // значение
  '_type': number
}[];

export type TBackendRequestAuthenticate = {
  login: string,
  password: string,
}

export type TBackendResponseAuthenticate = {
  error?: string,
  status?: string,
}

export type TBackendResponseAuthenticationStatus = {
  loggedin: 'web' | 'nothing'
}

export type TBackendResponseLogout = {
  error?: string,
  status?: string,
}

export type TBackendResponseMenu = {
  menu: {
    folder: {
      caption: string,
      file: {
        caption: string,
        command: string,
      }[]
    }[]
  }
};

export type TRetrospectiveSettings = {
  selectStartTime: Moment,
  selectEndTime: Moment,
  isAutoReloadEnabled: boolean,
  isLinePointsVisible: boolean,
  visualizationType: 'chart' | 'table'
};

export type TBackendResponseLicenses = Readonly<ILicense>;

export enum ERetrospectiveMeasureStyle {
  SOLID,
  DASHED_SHORT_LINES,
  DASHED_LONG_LINES,
  DASHED_VARIABLE_LENGTH_LINES
}
