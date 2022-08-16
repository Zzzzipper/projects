import React from 'react';
import { Menu } from 'antd';
import { NavLink } from 'react-router-dom';

import {
    SafetyCertificateOutlined,
} from '@ant-design/icons';

interface IState {
    activeKey: string;
}

class AppMenu extends React.Component {

    state: IState = {
        activeKey: '1',
    };

    handleSelect = (event: any) => {
        if (this.state.activeKey !== event.key) {
            this.setState({ activeKey: event.key });
        }
    };

    isActiveEvent = (activeKey: string, match: any) => {
        if (!match) {
            return false;
        }

        if (this.state.activeKey !== activeKey) {
            this.setState({ activeKey });
        }

        return true;
    };

    render() {
        return (
            <Menu theme="dark" mode="inline" >
                <Menu.Item key="license" icon={<SafetyCertificateOutlined />}>
                    <NavLink to="/license">
                        Лицензии
                    </NavLink>
                </Menu.Item>
            </Menu>
        );
    }
}

export default AppMenu;
