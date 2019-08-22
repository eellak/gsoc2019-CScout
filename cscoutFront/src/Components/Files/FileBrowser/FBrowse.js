import React, { Component } from 'react';
import axios from 'axios';
import '../../../global.js';
import Directory from './Directory';
import './FBrowse.css';
import Files from '../FilePage/FilePage'

class FBrowse extends Component {
    constructor(props) {
        super(props);
        this.state = {
            loaded: false,
            file: null,
            id: null
        }

    };

    componentDidMount() {
        this.getTopDir();
    }

    getTopDir = () => {
        if (this.props.type === "top") {
            axios.get(global.address + "browseTop")
                .then((response) => {
                    this.setState({
                        loaded: true,
                        top: response.data.addr,
                        name: response.data.info.name
                    });
                })
        }
    }

    getFileInfo = (param) => {
        this.setState({
            id: param
        })
    }

    render() {
        if (this.state.loaded === false){
            return (
                <div style={{cursor:"wait"}}>
                    <h2>
                        Loading...
                    </h2>
                </div>
            );
        }
        else {
            return (
                <div style={{ display: 'flex' }}>
                    <div className="FileBrowser">
                        <h3>File Browser</h3>
                        <Directory addr={this.state.top} name={this.state.name}
                            expand={true} fileSelect={this.getFileInfo} />
                    </div>
                    {
                        (this.state.id === null) ?
                            null
                            : <Files id={this.state.id}
                                changeType={this.props.changeType} />
                    }
                </div>
            );
        }
    }
}
export default FBrowse;