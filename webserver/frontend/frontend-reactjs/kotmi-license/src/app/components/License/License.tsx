import React from 'react';
import utils from '../../utils/objectutils';
import { AppState } from '../../../redux/reducers';
import { connect } from 'react-redux';

import { Card, Col, Row, Tree } from 'antd';

import {
   getLicense,
} from '../../../redux/actions/license';

import {
   ILicenseState,
} from '../../../redux/reducers/license/types';

interface IProps {
   license: ILicenseState;
};

interface IDispatchProps {
   getLicense: (param: object) => Promise<any>;
}

class License extends React.Component<IProps & IDispatchProps> {

   state = {
      panelTitle: null,
      panelData: null,
   };

   componentDidMount() {
      this.props.getLicense({});
   }

   onSelect = (selectedKeys: any, info: any) => {
      const panelData = utils.objKeyValToString(
         utils.findInObject(this.props.license, selectedKeys[0]));
      this.setState({
         panelTitle: info?.node?.title,
         panelData,
      });
   };

   render() {

      const { panelData, panelTitle } = this.state;
      const { license } = this.props;
      const treeData = (license == undefined || license?.kotmi_license == undefined) ? [] : [{
         title: "kotmi_license " + license?.kotmi_license?.number,
         key: "kotmi_license",
         children:
            license?.kotmi_license?.licenses?.map((item) => {
               return {
                  dataRef: item,
                  title: item.caption,
                  key: item.name,
                  children: (item.license?.map((item1) => {
                     return {
                        dataRef: item1,
                        title: item1.caption,
                        key: item1.name,
                     }
                  })
                  )
               }
            })
      }];

      return (
         <div className='App-body'>
            <Row>
               <Col span={12}>
                  <Tree
                     className='license-tree'
                     showLine
                     treeData={treeData}
                     onSelect={this.onSelect}
                  />
               </Col>
               <Col span={12}>
                  <Card title={panelTitle}>
                     <pre>
                        {panelData}
                     </pre>
                  </Card>
               </Col>
            </Row>
         </div >
      )
   }
}

function mapStateToProps(state: AppState): IProps {
   return {
      license: state.license.license,
   };
}

const mapDispatchToProps = {
   getLicense,
};

export default connect(
   mapStateToProps,
   mapDispatchToProps,
)(License);