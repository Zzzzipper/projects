import React from 'react';
import { connect } from 'react-redux';
import {
    Alert,
    Card,
    Col,
    Form,
    Input,
    Button,
    Layout,
    Row,
} from 'antd';

import {
    auth,
} from '../../redux/actions/core';

import {
    TAuthStatus,
} from '../../redux/reducers/core/types';

import { AppState } from '../../redux/reducers';

interface IProps {
    loggedin: TAuthStatus;
};

interface IDispatchProps {
    auth: (param: object) => Promise<any>;
    onAuth: () => any;
}

class AuthPage extends React.Component<IProps & IDispatchProps> {

    state = {
        isLoading: false,
        errorMsg: null,
    }

    login = (values: any) => {
        this.setState({
            isLoading: true,
        })
        this.props.auth(values).then((response) => {
            this.setState({ isLoading: false });
            if (response.payload.data?.code === 9) {
                this.props.onAuth();
            } else {
                this.setState({ errorMsg: 'Неправильный логин или пароль!' })
            }
        });
    }

    render() {

        return (
            <Layout style={{ display: 'flex', flexDirection: 'column', height: '100vh' }}>
                <Row
                    itemType='flex'
                    justify="center"
                    align="middle"
                    style={{ flexGrow: 1 }}
                >
                    <Col style={{ width: '400px' }}>
                        <Card
                            bordered
                            title='Вход'
                            headStyle={{ display: 'flex', justifyContent: 'center' }}
                        >
                            <Form
                                name="basic"
                                labelCol={{ span: 6 }}
                                wrapperCol={{ span: 18 }}
                                onFinish={this.login}
                                autoComplete="off"
                            >
                                <Form.Item
                                    label="Логин"
                                    name="login"
                                    rules={[{ required: true, message: 'Введите логин!' }]}
                                >
                                    <Input />
                                </Form.Item>

                                <Form.Item
                                    label="Пароль"
                                    name="password"
                                    rules={[{ required: true, message: 'Введите пароль!' }]}
                                >
                                    <Input.Password />
                                </Form.Item>

                                {this.state.errorMsg !== null ? (
                                    <Form.Item  wrapperCol={{ span: 24 }}>
                                        <Alert message={this.state.errorMsg} type="error" showIcon />
                                    </Form.Item>
                                ) : null}

                                <Form.Item style={{ marginBottom: 0 }} wrapperCol={{ span: 24 }}>
                                    <Row justify="center">
                                        <Button type="primary" htmlType="submit" loading={this.state.isLoading}>
                                            Войти
                                        </Button>
                                    </Row>
                                </Form.Item>
                            </Form>
                        </Card>
                    </Col>
                </Row>
            </Layout>
        )
    }
}
function mapStateToProps(state: AppState): IProps {
    return {
        loggedin: state.core.loggedin,
    };
}

const mapDispatchToProps = {
    auth,
};

export default connect(
    mapStateToProps,
    mapDispatchToProps,
)(AuthPage);
