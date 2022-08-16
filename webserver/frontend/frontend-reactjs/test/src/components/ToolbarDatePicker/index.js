import React from 'react';
import moment from 'moment';

import { DatePicker, Tooltip } from 'antd';

export const ToolbarDatePicker = ({ caption, hint, format, value, onChange, onOpenChange }) => {

    return (
        <React.Fragment>
            <span style={caption ? { paddingRight: '5px' } : { paddingRight: 0 }}>{caption}</span>
            <Tooltip placement='bottomLeft' title={hint}
                color='rgb(255,254,189)' overlayInnerStyle={{ color: 'black' }}>
                <DatePicker format={format} showTime allowClear={false}
                    value={moment(value, format)}
                    onChange={onChange} onOpenChange={onOpenChange} >
                </DatePicker>
            </Tooltip>
        </React.Fragment>
    )
};



/*

<Tooltip placement='bottomLeft' title={hint}
            color='rgb(255,254,189)' overlayInnerStyle={{ color: 'black' }}>
            <div style={{ display: 'inline' }}>
                <span style={caption ? { paddingRight: '5px' } : { paddingRight: 0 }}>{caption}</span>
                <DatePicker format={format} showTime allowClear={false}></DatePicker>
            </div>
        </Tooltip>
*/
