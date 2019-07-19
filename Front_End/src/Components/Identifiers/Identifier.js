import React, {Component} from 'react';
import Axios from 'axios';
import '../../global.js';
import Table from '../Table';

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

    
    
    getDetails(){
        return(
            <div>
                <h3>
                    Details
                </h3>
                <table style={{textAlign:'left'}}>

                    <tbody>
                    {
                        this.state.data.attribute.map((obj,i) => 
                        <tr key={i}>
                            <td>
                                {obj.name}
                            </td>
                            <td>
                                {obj.get.toString()}
                            </td>
                        </tr>
                        )
                    }
                    </tbody>
                </table>
            </div>
        )
    }

    render(){
        return(
            <div>
                {this.state.loaded?<div>{
                    //JSON.stringify(this.state.data.attribute)
                    this.getDetails()
                }</div>:<div>Loading ... </div>}
            </div>
        )
    }
}
export default Identifier;