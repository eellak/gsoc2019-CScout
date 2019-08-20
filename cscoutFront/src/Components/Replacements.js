import React,{Component} from'react';
import axios from 'axios';
import '../global.js';
import Table from './Table';

class Replacements extends Component{
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
        axios.get(global.address + "replacements.html")
        .then((response) =>
            this.setState({
                data: response.data,
                loaded:true,
                ref: Array.isArray(response.data.content)?response.data.content.map((obj,i) => obj.active):[],
                repl: Array.isArray(response.data.content)?response.data.content.map((obj,i) => obj.new_id):[]
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
        this.state.data.content.forEach((obj,i) => 
            {
                g[obj.id_address] = {
                    repl: this.state.repl[i],
                    active: this.state.ref[i]
                }
            }
            )
        axios.put(global.address + "xreplacements.html", g, {
            headers: {
                "Accept": "application/json",
                "Content-type": "application/json",
                "Access-Control-Allow-Origin":"*",
                "Access-Control-Allow-Headers": "Content-Type, Accept, Access-Control-Allow-Origin"
            }
        })
        .then((response) => {
            console.log(response);
            this.props.openModal(<div>{response.data.ok?"Identifier Replacements OK":"Error in identifier Replacement:" + response.data.error}</div>)
            }
        )
    }

    render(){
        return(
            <div>
                {!this.state.loaded?
                    <div>Loading...</div>
                :<div className="refactors">
                    <h3>Identifier Replacements</h3>

                   {Array.isArray(this.state.data.content)?
                    <div>
                        <Table head={["Identifier","Replacement","Active"]} contents={this.state.data.content.map((obj, i) =>
                               [<a onClick={() => this.props.changeType("id",obj.id_address)}>{obj.name}</a>,
                                <input type="text" value={this.state.repl[i]} key={i} onChange={(e) => this.handleChange(e,i)}/>,
                               <input type="checkbox" checked={this.state.ref[i]} onChange={() => {
                                   var refs = this.state.ref;
                                   refs[i] = !refs[i]
                                   this.setState({ref: refs})} 
                                } />
                            ] 
                            )}/>
                        <button onClick={this.handleSubmit} className="formButton">OK</button>
                    </div>
                    :<div>No Identifier Replacements</div>}
                </div>
                
                }
            </div>
        )
    }
}

export default Replacements;