import React,{Component} from 'react';
import axios from 'axios';
import '../../global.js';
import Directory from './Directory';
import './FBrowse.css';


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
                <div>
                    <div className="FileBrowser">
                        <h3>File Browser</h3>   
                        <Directory addr={this.state.top} name={this.state.name} 
                        expand={true} fileSelect={this.getFileInfo}/>
                    </div>
                    <div className="Fileinfo">
                        {(this.state.file===null)?<p>No file selected</p>:<p>{JSON.stringify(this.state.file, null, 2)}</p>}
                    </div>
                </div>
            );
    }
}
export default FBrowse;