import { ILicense } from '../entities/licenses';

export const data: ILicense = {
  "kotmi_license": {
    "number": 46481,
    "date": "2021.05.12",
    "limit": 12,
    "version": "2.1.0",
    "vendor": {
      "name": "Алтай-Энерго"
    },
    "client": {
      "name": "Комплект №1"
    },
    "licenses": [
      {
        "name": "DmsArm.exe",
        "caption": "DMS. АРМ аналитика (режимщика, offline)",
        "isValid": 0,
        "lastError": "Empty license",
        "param": [
          {
            "name": "arm_max",
            "caption": "Кол. АРМ аналитика",
            "value": 5,
            "currentValue": 0,
            "isValid": 0,
            "lastError": "Empty parameter"
          }
        ],
        "license": [
          {
            "name": "DmsDivPointOff.exe",
            "caption": "Определение точек деления сети",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "DmsEnReliabOff.exe",
            "caption": "Анализ надежности энергоснабжения",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "DmsEstimateOff.exe",
            "caption": "Оценивание состояния",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "DmsLossesOff.exe",
            "caption": "Расчет потерь электроэнергии",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "DmsN1Off.exe",
            "caption": "Анализ надежности режима (N-1)",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "DmsOptimOff.exe",
            "caption": "Оптимизация режима по реактивной мощности",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "DmsPlanOff.exe",
            "caption": "Планирование развития сети",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "DmsPrognosOff.exe",
            "caption": "Прогноз потребления",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "DmsRepairOff.exe",
            "caption": "Восстановление энергоснабжения после аварии",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "DmsSerialOff.exe",
            "caption": "Серийные расчеты'",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "DmsTkzOff.exe",
            "caption": "Расчет токов короткого замыкания (ТКЗ)",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "DmsUrOff.exe",
            "caption": "Расчет установившегося режима",
            "isValid": 0,
            "lastError": "Empty license"
          }
        ]
      },
      {
        "name": "DmsCalcModel.exe",
        "caption": "DMS. Формирование расчетной модели (online)",
        "isValid": 0,
        "lastError": "Empty license",
        "license": [
          {
            "name": "DmsDivPointOn.exe",
            "caption": "Определение точек деления сети",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "DmsEnReliabOn.exe",
            "caption": "Анализ надежности энергоснабжения",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "DmsEstimateOn.exe",
            "caption": "Оценивание состояния",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "DmsLossesOn.exe",
            "caption": "Расчет потерь электроэнергии",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "DmsN1On.exe",
            "caption": "Анализ надежности режима (N-1)",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "DmsOptimOn.exe",
            "caption": "Оптимизация режима по реактивной мощности",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "DmsPlanOn.exe",
            "caption": "Планирование развития сети",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "DmsPrognosOn.exe",
            "caption": "Прогноз потребления",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "DmsRepairOn.exe",
            "caption": "Восстановление энергоснабжения после аварии",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "DmsSerialOn.exe",
            "caption": "Серийные расчеты'",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "DmsTkzOn.exe",
            "caption": "Расчет токов короткого замыкания (ТКЗ)",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "DmsUrOn.exe",
            "caption": "Расчет установившегося режима",
            "isValid": 0,
            "lastError": "Empty license"
          }
        ]
      },
      {
        "name": "RdxServer.exe",
        "caption": "Сервер ввода-вывода",
        "isValid": 0,
        "lastError": "Empty license",
        "param": [
          {
            "name": "dir_max",
            "caption": "Макс. кол. направлений",
            "value": 5,
            "currentValue": 0,
            "isValid": 0,
            "lastError": "Empty parameter"
          },
          {
            "name": "prm_max",
            "caption": "Максимальное количество принимаемых параметров",
            "value": 10000,
            "currentValue": 0,
            "isValid": 0,
            "lastError": "Empty parameter"
          }
        ],
        "license": [
          {
            "name": "RdxIec101.dll",
            "caption": "Протокол МЭК-101",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "RdxIec103.dll",
            "caption": "Протокол МЭК-103",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "RdxIec104.dll",
            "caption": "Протокол МЭК-104 (60870-5-104)",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "RdxIec61850.dll",
            "caption": "Протокол МЭК-61850 (61850-8.1 Client)",
            "isValid": 0,
            "lastError": "Empty license"
          }
        ]
      },
      {
        "name": "ScdAsureo.exe",
        "caption": "Интеграция с АСУРЭО",
        "isValid": 0,
        "lastError": "Empty license"
      },
      {
        "name": "ScdCalls.exe",
        "caption": "Обработка и учет звонков потребителей об отключениях",
        "isValid": 0,
        "lastError": "Empty license"
      },
      {
        "name": "ScdGisServer.exe",
        "caption": "Работа с геоданными на формах отображения. Гео-сервисы",
        "isValid": 0,
        "lastError": "Empty license"
      },
      {
        "name": "ScdSap.exe",
        "caption": "Интеграция с SAP TORO",
        "isValid": 0,
        "lastError": "Empty license"
      },
      {
        "name": "ScdServer.exe",
        "caption": "Сервер приложений",
        "isValid": 0,
        "lastError": "Empty license",
        "param": [
          {
            "name": "arm_max",
            "caption": "Макс. кол. одновременно подключенных АРМ",
            "value": 5,
            "currentValue": 0,
            "isValid": 0,
            "lastError": "Empty parameter"
          },
          {
            "name": "prm_max",
            "caption": "Макс. кол. принимаемых параметров",
            "value": 10000,
            "currentValue": 0,
            "isValid": 0,
            "lastError": "Empty parameter"
          }
        ],
        "license": [
          {
            "name": "ScdComtrade.ocx",
            "caption": "Осциллограммы",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "ScdLayoutMail.exe",
            "caption": "Чтение макетной информации из почты",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "ScdOperLog.ocx",
            "caption": "Электронный оперативный журнал",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "ScdReport.exe",
            "caption": "Генератор отчетов",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "ScdSwitchover.ocx",
            "caption": "Графики временных отключений (ГВО)",
            "isValid": 0,
            "lastError": "Empty license"
          },
          {
            "name": "Topological_processor",
            "caption": "Топологический процессор",
            "isValid": 0,
            "lastError": "Empty license"
          }
        ]
      },
      {
        "name": "ScdTraining.exe",
        "caption": "Тренажер диспетчера электрических сетей",
        "isValid": 0,
        "lastError": "Empty license",
        "param": [
          {
            "name": "instructor_max",
            "caption": "Макс. кол. инструкторов",
            "value": 5,
            "currentValue": 0,
            "isValid": 0,
            "lastError": "Empty parameter"
          },
          {
            "name": "trainee_max",
            "caption": "Макс. кол. тренируемых",
            "value": 20,
            "currentValue": 0,
            "isValid": 0,
            "lastError": "Empty parameter"
          }
        ]
      },
      {
        "name": "ScdWebServer.exe",
        "caption": "WEB-доступ",
        "isValid": 0,
        "lastError": "Empty license",
        "param": [
          {
            "name": "user_max",
            "caption": "Макс. кол. одновременно подключенных пользователей",
            "value": 10,
            "currentValue": 0,
            "isValid": 0,
            "lastError": "Empty parameter"
          }
        ]
      },
      {
        "name": "information_model",
        "caption": "Информационная модель",
        "isValid": 0,
        "lastError": "Empty license",
        "param": [
          {
            "name": "ps_110_220",
            "caption": "ПС 110-220",
            "value": 5,
            "currentValue": 0,
            "isValid": 0,
            "lastError": "Empty parameter"
          },
          {
            "name": "ps_330_500",
            "caption": "ПС 330-500",
            "value": 6,
            "currentValue": 0,
            "isValid": 0,
            "lastError": "Empty parameter"
          },
          {
            "name": "ps_35",
            "caption": "ПС 35",
            "value": 4,
            "currentValue": 0,
            "isValid": 0,
            "lastError": "Empty parameter"
          },
          {
            "name": "recloser",
            "caption": "Реклоузеры",
            "value": 3,
            "currentValue": 0,
            "isValid": 0,
            "lastError": "Empty parameter"
          },
          {
            "name": "rp_6_20",
            "caption": "РП 6-20",
            "value": 2,
            "currentValue": 0,
            "isValid": 0,
            "lastError": "Empty parameter"
          },
          {
            "name": "tp_6_20",
            "caption": "ТП 6-20",
            "value": 1,
            "currentValue": 0,
            "isValid": 0,
            "lastError": "Empty parameter"
          }
        ]
      }
    ]
  }
}
