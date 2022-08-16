import React from 'react';
import { connect } from 'react-redux';
import { Menu } from 'antd';
import { NavLink } from 'react-router-dom';

import {
    LogoutOutlined,
} from '@ant-design/icons';

import { IMenu } from '../../redux/reducers/core/types';
import { AppState } from '../../redux/reducers';

interface IProps {
    menu: IMenu | undefined;
};

interface IState {
    activeKey: string;
}

class AppMenu extends React.Component<IProps, IState> {

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

        const { menu } = this.props;

        const dynamicMenu = menu?.menu?.map((item) => {
            return (
                <Menu.SubMenu key={item?.caption} title={item?.caption}>
                    {item?.menu?.map((panel) => {
                        return (
                            <Menu.Item
                                key={panel?.command}
                            >
                                <NavLink
                                    to={`/retrospective/${panel?.command}`}
                                >
                                    {panel?.caption}
                                </NavLink>
                            </Menu.Item>
                        )
                    })}
                </Menu.SubMenu>
            )
        });

        // const openKeys = Array<string>(this.props.menu?.folder[0]) || Array<string>();

        return (
            <Menu
                // defaultOpenKeys={['sub1']}
                // onClick={this.handleSelect}
                theme="dark"
                mode="inline"
            >
                {dynamicMenu}
                <Menu.Item key="logout" icon={<LogoutOutlined />}>
                    <NavLink
                        // isActive={(match) => this.isActiveEvent('2', match)}
                        to="/authentication"
                    >
                        Выйти
                    </NavLink>
                </Menu.Item>
            </Menu>
        );
    }
}

function mapStateToProps(state: AppState): IProps {
    return {
        menu: state.core.menu,
    };
}

export default connect(
    mapStateToProps,
    null,
)(AppMenu);
