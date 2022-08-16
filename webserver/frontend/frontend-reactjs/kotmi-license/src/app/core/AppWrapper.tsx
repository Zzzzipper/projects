import React from 'react';
import { RouteComponentProps } from "react-router";

import { withRouter } from 'react-router-dom';

import { Layout } from 'antd';

import {
    MenuUnfoldOutlined,
    MenuFoldOutlined,
} from '@ant-design/icons';

import './styles.less';
import 'antd/dist/antd.less';

import AppRouter from './AppRouter';
import AppMenu from './AppMenu';


const { Header, Sider, Content } = Layout;

class Wrapper extends React.Component<RouteComponentProps> {

    state = {
        collapsed: false,
    };

    toggle = () => {
        this.setState({
            collapsed: !this.state.collapsed,
        });
    };

    render() {

        return (
            <Layout>
                <Sider trigger={null} collapsible collapsed={this.state.collapsed}>
                    <div className="logo">
                        <h1>АРМ КОТМИ</h1>
                    </div>
                    <AppMenu/>
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
            </Layout>
        );
    }
}

export default withRouter(Wrapper);
