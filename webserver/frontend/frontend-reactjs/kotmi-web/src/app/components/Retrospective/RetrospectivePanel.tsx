import React from 'react';
import moment from 'moment';

import {
   CaretLeftOutlined,
   CaretRightOutlined,
   CheckOutlined,
   ClockCircleOutlined,
   DotChartOutlined,
   DragOutlined,
   FilePdfOutlined,
   LineChartOutlined,
   ReloadOutlined,
   SettingOutlined,
   TableOutlined,
   ZoomInOutlined,
   ZoomOutOutlined,
} from '@ant-design/icons';

import {
   DatePicker,
   Tooltip,
} from 'antd';

class RetrospectivePanel extends React.Component {

   render() {
      return (
         <div className='retro-panel'>
            <Tooltip title="Таблица">
               <TableOutlined className='retro-panel-element' />
            </Tooltip>
            <Tooltip title="Диаграммы">
               <LineChartOutlined className='retro-panel-element' />
            </Tooltip>
            <span style={{ marginLeft: '15px', fontSize: '15px' }}>
               Время ретроспективы:
            </span>
            <DatePicker.RangePicker size='small' style={{ marginLeft: '5px' }} />
            <Tooltip title="Шаг по времени назад">
               <CaretLeftOutlined className='retro-panel-element' />
            </Tooltip>
            <Tooltip title="Шаг по времени вперед">
               <CaretRightOutlined className='retro-panel-element' style={{ marginLeft: '5px' }} />
            </Tooltip>
            <Tooltip title="Прокрутка по времени вручную">
               <DragOutlined className='retro-panel-element' />
            </Tooltip>
            <Tooltip title="Обновить за текущее время">
               <ReloadOutlined className='retro-panel-element' />
            </Tooltip>
            <Tooltip title="Автоматическое обновление">
               <ClockCircleOutlined className='retro-panel-element' />
            </Tooltip>
            <Tooltip title="Настройки">
               <SettingOutlined className='retro-panel-element' />
            </Tooltip>
            <Tooltip title="Сигналы">
               <CheckOutlined className='retro-panel-element' />
            </Tooltip>
            <Tooltip title="Точки графика">
               <DotChartOutlined className='retro-panel-element' />
            </Tooltip>
            <Tooltip title="Увеличить масштаб">
               <ZoomInOutlined className='retro-panel-element' />
            </Tooltip>
            <Tooltip title="Уменьшить масштаб">
               <ZoomOutOutlined className='retro-panel-element' />
            </Tooltip>
            <Tooltip title="Экспорт в PDF">
               <FilePdfOutlined className='retro-panel-element-pdf' />
            </Tooltip>
         </div>
      )
   }
}
export default RetrospectivePanel;
