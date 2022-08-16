import { App } from 'vue';
// import Alert from 'ant-design-vue/lib/alert';
// import Button from 'ant-design-vue/lib/button';
// import DatePicker from 'ant-design-vue/lib/date-picker';
// import Spin from 'ant-design-vue/lib/spin';
// import Layout from 'ant-design-vue/lib/layout';
// import Menu from 'ant-design-vue/lib/menu';
// import Skeleton from 'ant-design-vue/lib/skeleton';
// import Row from 'ant-design-vue/lib/row';
// import Col from 'ant-design-vue/lib/col';
// import Form from 'ant-design-vue/lib/form';
// import Input from 'ant-design-vue/lib/input';
// import Card from 'ant-design-vue/lib/card';
// import Space from 'ant-design-vue/lib/space';
// import Select from 'ant-design-vue/lib/select';
import { Alert, Button, DatePicker, Spin, Layout, Menu, Skeleton, Row, Col, Form, Input, Card, Space, Select, Modal, List, Checkbox, Tag } from 'ant-design-vue';
import './index.less';

export function useAnt (app: App) {
  app.use(DatePicker);
  app.use(Button);
  app.use(Alert);
  app.use(Spin);
  app.use(Layout);
  app.use(Menu);
  app.use(Skeleton);
  app.use(Row);
  app.use(Col);
  app.use(Form);
  app.use(Input);
  app.use(Card);
  app.use(Space);
  app.use(Select);
  app.use(Modal);
  app.use(List);
  app.use(Checkbox);
  app.use(Tag);
}
