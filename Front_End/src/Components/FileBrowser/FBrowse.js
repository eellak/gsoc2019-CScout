import React,{Component} from 'react';
import axios from 'axios';
import '../../global.js';
import Directory from './Directory';


class FBrowse extends Component{
    constructor(props) {
        super(props);
        this.state={
            loaded:false
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
                    <Directory addr={this.state.top} name={this.state.name} expand={true}/>
                </div>
            );
    }
}
export default FBrowse;