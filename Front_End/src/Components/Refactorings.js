import React,{Component} from'react';
import axios from 'axios';
import '../global.js';
import Table from './Table';
import './Refactorings.css';

class Refactorings extends Component{
    constructor(props){
        super(props)
        this.state = { 
            loaded: false
        }
        this.handleChange = this.handleChange.bind(this)
        this.handleSubmit = this.handleSubmit.bind(this)
    }


    componentDidMount(){
        this.getRefacts();
    }

    getRefacts() {
        axios.get(global.address + "funargrefs.html")
        .then((response) =>
            this.setState({
                data: response.data,
                loaded:true,
                ref: Array.isArray(response.data.data)?response.data.data.map((obj,i) => obj.active):[],
                repl: Array.isArray(response.data.data)?response.data.data.map((obj,i) => obj.replacement):[]
            },()=> console.log(this.state))
        )
    }

    handleChange(e,i) {
        console.log(e.target)
        var l = this.state.repl;
        l[i] = e.target.value;
        this.setState({
            repl: l
        })
    }

    handleSubmit(){
        var g={};  
        this.state.data.data.forEach((obj,i) => 
            {
                g[obj.ec] = {
                    repl: this.state.repl[i],
                    active: this.state.ref[i]
                }
            }
            )
        axios.put(global.address + "xfunargrefs.html", g, {
            headers: {
                "Accept": "application/json",
                "Content-type": "application/json",
                "Access-Control-Allow-Origin":"*",
                "Access-Control-Allow-Headers": "Content-Type, Accept, Access-Control-Allow-Origin"
            }
        })
        .then((response) => 
            console.log(response)
        )
    }

    render(){
        return(
            <div>
                {!this.state.loaded?
                    <div>Loading...</div>
                :<div className="refactors">
                   {Array.isArray(this.state.data.data)?
                    <div>
                        <Table head={["Function","Arguments","Active"]} contents={this.state.data.data.map((obj, i) =>
                               [<a onClick={() => this.props.changeType("fun",obj.f)}>{obj.name}</a>,
                                <input type="text" value={this.state.repl[i]} key={i} onChange={(e) => this.handleChange(e,i)}/>,
                               <input type="checkbox" checked={this.state.ref[i]} onChange={() => {
                                   var refs = this.state.ref;
                                   refs[i] = !refs[i]
                                   this.setState({ref: refs})} 
                                } />
                            ] 
                            )}/>
                        <button onClick={this.handleSubmit}>OK</button>
                    </div>
                    :<div>No Refactorings</div>}
                </div>
                
                }
            </div>
        )
    }
}

export default Refactorings;