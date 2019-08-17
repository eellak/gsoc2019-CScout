import React,{Component} from'react';
import axios from 'axios';
import '../global.js';

class SelectProj extends Component{
    constructor(props){
        super(props)
        this.state = { 
            loaded: false
        }
       
    }


    componentDidMount(){
        this.getProjs();
    }

    getProjs() {
        axios.get(global.address + "sproject.html")
        .then((response) =>
            this.setState({
                data: response.data,
                loaded:true,
            },()=> console.log(this.state))
        )
    }

    setProj(p){
        axios.get(global.address + "setproj.html?projid=" + p).
        then( (r) =>
            this.setState({changed: r})
        )
    }

    render(){
        return(
            <div>
                {!this.state.loaded?
                    <div>Loading...</div>
                :<div className="sproj">
                    {this.state.changed?<div>{this.state.changed.ok?"Project set":"Error "+this.state.changed.error}</div>
                    :<div>
                        <div onClick={() => this.setProj(0)} style={{cursor:"pointer"}}>All projects</div>
                        {Object.keys(this.state.data).map((key,i) =>
                            <div onClick={() => this.setProj(this.state.data[key])} key={i} style={{cursor:"pointer"}}>{key}</div>
                        )}
                    </div>}
                </div>
                
                }
            </div>
        )
    }
}

export default SelectProj;