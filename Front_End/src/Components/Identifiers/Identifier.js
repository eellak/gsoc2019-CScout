import React, {Component} from 'react';
import Axios from 'axios';
import '../../global.js'

class Identifier extends Component{
    constructor(props){
        super(props);
        this.state = {
            loaded:false
        } 

    }

    componentDidMount(){
        this.getIdentifier();
    }

    getIdentifier(){
        Axios.get(global.address+'id.html?id='+this.props.id)
        .then((response) => {
            this.setState({
                loaded:true,
                data:response.data
            })
        })
    }

    render(){
        return(
            <div>
                {this.state.loaded?<div>{JSON.stringify(this.state.data)}</div>:<div>Loading ... </div>}
            </div>
        )
    }
}
export default Identifier;