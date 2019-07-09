import React,{Component} from 'react';
import axios from 'axios';
import '../../global.js';
import Directory from './Directory';
import './FBrowse.css';
import Table from '../Table'


class FBrowse extends Component{
    constructor(props) {
        super(props);
        this.state={
            loaded:false,
            file:null
        }
        
    };

    componentDidMount() {
        this.getTopDir();
    }


    getTopDir = () => {
        if(this.props.type === "top"){
            axios.get(global.address + "browseTop.html")
            .then((response) => {
                this.setState({
                    loaded:true,
                    top: response.data.addr,
                    name: response.data.info.name
                });
            })
        }     
    }

    getFileInfo = (param) => {
        axios.get(global.address + "file.html?id="+param)
        .then((response) => {
            this.setState({
                file: response.data
            })
            console.log(Object.keys(this.state.file.metrics));
        })
    }



    render() {
        console.log(this.state);
        if (this.state.loaded===false)
            return(
                <div>
                    <h2>
                        Loading...
                    </h2>
                </div>
            );
        else
            return (
                <div style={{display:'flex'}}>
                    <div className="FileBrowser">
                        <h3>File Browser</h3>   
                        <Directory addr={this.state.top} name={this.state.name} 
                        expand={true} fileSelect={this.getFileInfo}/>
                    </div>
                    <div className="FileInfo">
                        {(this.state.file===null)?<p>No file selected</p>
                        :<div>
                            <h2>
                                {this.state.file.pathname}
                            </h2>
                            <Table head={["Metrics","Values"]} contents={this.state.file.metrics}/>
                        </div>
                    }
                    </div>
                </div>
            );
    }
}
export default FBrowse;