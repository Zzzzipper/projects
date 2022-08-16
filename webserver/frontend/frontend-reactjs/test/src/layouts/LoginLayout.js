import React, { useState } from 'react';
import { Form, Input, Button, Checkbox, Layout, Row, Col, Card, Alert } from 'antd';

import 'antd/dist/antd.less';

const { Content } = Layout;

import { login } from '../common/Api';

function LoginLayout({ onLoggedIn }) {

    const [alertText, setAlertText] = useState('');

    const onFinish = (values) => {
        let isLoggedIn = false;

        console.log('login click');

        login(values.username, values.password).then(result => {
            let newAlertText = '';

            if (result.code < 0) {
                const idx = result.message.indexOf('desc');
                if (idx < 0)
                    newAlertText = result.message;
                else // выводим в сообщение все, что после 'desc = '
                    newAlertText = result.message.slice(idx + 7);
            }
            else
                isLoggedIn = true;

            setAlertText(newAlertText);
            if (isLoggedIn && onLoggedIn)
                onLoggedIn();
        });

    };

    return (
        <Layout className='layout' style={{ height: '100vh' }}>
            <Content className='site-layout-content' style={{ display: 'flex' }}>
                <Row className='ant-row-middle ant-row-center' style={{ flexGrow: 1 }}>
                    <Col style={{ width: 360 }}>
                        <Card title='КОТМИ-2014. Вход в систему' bordered={false}>
                            <Form initialValues={{ username: '', password: '' }} requiredMark={false} labelCol={{ span: 8 }} labelAlign='left'
                                onFinish={onFinish}>
                                <Form.Item label='Пользователь' name='username'
                                    rules={[{ required: true, message: 'Обязательно для заполнения!' }]}
                                >
                                    <Input></Input>
                                </Form.Item>
                                <Form.Item label='Пароль' name='password'>
                                    <Input.Password></Input.Password>
                                </Form.Item>
                                <Form.Item hidden={alertText ? false : true}>
                                    <Alert type='error' showIcon message={alertText}></Alert>
                                </Form.Item>
                                <Form.Item style={{ span: 24, textAlign: 'right', marginBottom: 0 }}>
                                    <Button htmlType='submit' style={{ width: 100 }}>ОК</Button>
                                </Form.Item>
                            </Form>
                        </Card>
                    </Col>
                </Row>
            </Content>
        </Layout >
    )
};

export default LoginLayout;

