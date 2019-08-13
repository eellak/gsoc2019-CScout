import React, { Component } from 'react';
import Tabs from '../../Tabs/Tabs';
import Details from './Details';
import SourceControl from '../Source/SourceControl';
import Table from '../../Table';
import axios from 'axios';
import './FilePage';
import FunctionSearch from '../../Functions/FunctionSearch';
import ReactSVG from 'react-svg';

class Files extends Component {
    constructor(props) {
        super(props);
        this.state = {
            loaded: false,
            file: null
        }

    };

    componentDidMount() {
        this.getFileInfo();
    }

    componentDidUpdate(prevProps){
        if (prevProps.id !== this.props.id) {
            this.getFileInfo()
        }
    }

    getFileInfo = () => {
        axios.get(global.address + "file.html?id=" + this.props.id)
            .then((response) => {
                this.setState({
                    file: response.data,
                    loaded: true
                })

            })
    }

    contentClickHandler = (e) => {
        const targetLink = e.target.closest('a');
        if(!targetLink) return;
        console.log(targetLink.getAttribute("xlink:href"));
        this.props.changeType("fun",targetLink.getAttribute("xlink:href"));
        e.preventDefault();
    };

    render() {
        if (this.state.loaded === false)
            return (
                <div style={{cursor:"wait"}}>
                    <h2>
                        Loading...
                </h2>
                </div>
            );
        else {
            var tabs = {}
            if (this.state.file !== null)
                tabs = [
                    {
                        title: "Details",
                        content: <Details dets={this.state.file} />
                    },
                    {
                        title: "Metrics",
                        content: <Table head={["Metrics", "Values"]} contents={this.state.file.metrics} />
                    },
                    {
                        title: "Source",
                        content: <SourceControl id={this.state.file.queries.id} changeType={this.props.changeType} />
                    },
                    {
                        title: "Functions",
                        content: <FunctionSearch url={"fid=" + this.props.id} changeType={this.props.changeType} />
                    },
                    {
                        title: "Function Macro Graph",
                        content: <ReactSVG src={global.address + "cgraph.svg?all=1&fid=" + this.props.id} 
                                    onClick={this.contentClickHandler}  className="svgCont" 
                                    loading={() => {
                                        document.body.style.cursor="wait";
                                        return<div>Loading...</div>;
                                        }} 
                                    afterInjection={(err, svg) => {
                                        if (err) {
                                            console.error(err)
                                            return
                                        } 
                                            document.body.style.cursor="default";
                                        }}
                                    />
                    }
                ];
            console.log(tabs)
            return (
                <div className="FileInfo">
                    {(this.state.file === null) ? <p>No file selected</p>
                        : <div>
                            <h2>
                                {this.state.file.pathname}
                            </h2>

                            <Tabs children={tabs} />
                        </div>
                    }
                </div>
            );
        }
    }
}
export default Files;