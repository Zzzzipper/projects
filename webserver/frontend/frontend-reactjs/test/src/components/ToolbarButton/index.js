import React from 'react';

import { Button, Tooltip } from 'antd';

export const ToolbarButton = ({ icon, caption, hint, checked, onClick }) => {

    const btnOnClick = (event) => {
        event.preventDefault();

        if (onClick)
            onClick(event);
    };

    return (
        <Tooltip placement='bottomLeft' title={hint}
            color='rgb(255,254,189)' overlayInnerStyle={{ color: 'black' }}>
            <Button type='link' className={checked ? 'checked' : 'unchecked'} onClick={btnOnClick}>
                <img src={'./icons/' + icon} style={caption ? { paddingRight: '5px' } : { paddingRight: 0 }} />
                {caption}
            </Button>
        </Tooltip>
    )
}
