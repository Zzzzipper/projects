import React from 'react';
import { RouteComponentProps } from "react-router";

import { withRouter } from 'react-router-dom';
import { connect } from 'react-redux';

import { Layout } from 'antd';

import {
    MenuUnfoldOutlined,
    MenuFoldOutlined,
} from '@ant-design/icons';

import AppRouter from './AppRouter';
import AppMenu from './AppMenu';
import AuthPage from './AuthPage';

import {
    createModule,
    getAuthStatus,
    getMenu,
    logout,
} from '../../redux/actions/core';

import {
    TAuthStatus,
    TLogout,
    IMenu,
    TModule,
} from '../../redux/reducers/core/types';

import { AppState } from '../../redux/reducers';

interface IProps {
    loggedin: TAuthStatus;
    isLogout: TLogout;
    menu: IMenu | undefined;
    module: TModule;
};

interface IDispatchProps {
    // auth: (param: object) => Promise<any>;
    logout: () => void;
    getAuthStatus: (param: object) => Promise<any>;
    getMenu: (mod_id: any) => Promise<any>;
    createModule: () => Promise<any>;
}

const { Header, Sider, Content } = Layout;

class Wrapper extends React.Component<RouteComponentProps & IProps & IDispatchProps> {

    state = {
        collapsed: false,
    };

    static defaultProps: IProps = {
        loggedin: 'nothing',
        isLogout: {},
        menu: {
            menu: [],
            pict: {},
        },
        module: {
            mod_id: null,
            obj_with_mod_id: null,
        },
    };

    async componentDidMount() {
        await this.props.getAuthStatus({})
            .then((response) => {
                if (response.payload.data.code === 9) {
                    this.onAuth();
                }
            });
    }

    toggle = () => {
        this.setState({
            collapsed: !this.state.collapsed,
        });
    };

    onAuth = () => {
        this.props.createModule()
            .then(() => {
                this.props.getMenu(this.props.module?.mod_id);
            });
    };

    render() {

        const { menu, loggedin } = this.props;

        return (
            (loggedin != 'nothing') ? (
                <Layout>
                    <Sider trigger={null} collapsible collapsed={this.state.collapsed}>
                        <div className="logo">
                            <h1>АРМ КОТМИ</h1>
                        </div>
                        <AppMenu menu={menu} />
                    </Sider>
                    <Layout className="site-layout">
                        <Header className="site-layout-background" style={{ padding: 0 }}>
                            {React.createElement(this.state.collapsed ? MenuUnfoldOutlined : MenuFoldOutlined, {
                                className: 'trigger',
                                onClick: this.toggle,
                            })}
                        </Header>
                        <Content
                            className="site-layout-background"
                            style={{
                                margin: '24px 16px',
                                padding: 24,
                                minHeight: 280,
                            }}
                        >
                            <AppRouter />
                        </Content>
                    </Layout>
                </Layout>)
                : <AuthPage onAuth={this.onAuth} />
        );
    }
}

function mapStateToProps(state: AppState): IProps {
    return {
        loggedin: state.core.loggedin,
        isLogout: state.core.isLogout,
        menu: state.core.menu,
        module: state.core.module,
    };
}

const mapDispatchToProps = {
    createModule,
    getAuthStatus,
    getMenu,
    logout,
};

export default withRouter(connect(
    mapStateToProps,
    mapDispatchToProps,
)(Wrapper));
