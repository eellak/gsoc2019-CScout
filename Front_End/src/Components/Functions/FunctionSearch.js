import React,{Component} from 'react';
import Axios from 'axios';
import '../../global.js';

class FunctionSearch extends Component {
    constructor(props) {
        super(props);
        this.state = {
            loaded: false,
            size: 20,
            page: 0,
            orderby: "",
            orderField: 0,
            selectedOption: "all",
            rev: false
        }
        // this.objectComp = this.objectComp.bind(this);
        // this.handleOptionChange = this.handleOptionChange.bind(this);
        // this.handleSubmit = this.handleSubmit.bind(this);
        // this.handleInputChange = this.handleInputChange.bind(this);
        // this.inputValue = '';
        // this.maxShow = 20;
    }

    componentDidMount() {
        this.getFuns();
    }


    changeOrder(e) {
        if (this.state.orderField !== e) {
            this.setState({
                loaded: false,
                orderField: e,
                rev: false
            }, this.getFuns)
        }
        else {
            if (this.state.rev) {
                this.setState({
                    rev: false,
                    orderField: 0
                }, this.getFuns)

            }
            else
                this.setState({
                    rev: !this.state.rev
                }, this.getFuns);
        }
    }

    getFuns() {
        console.log(this.state)
        var url = "";
        url += this.state.writable?"&writable=1":"";
        url += this.state.ro?"&ro=1":"";
        url += this.state.cfun?"&cfun=1":"";
        if(url === "")
            url = "writable=1&ro=1&match=Y&ncallerop=0&n=All+Functions&qi=x";
        else {
            if(this.state.match === undefined)
                url += "&match=L";
            else 
                url += "&match=" + this.state.match;
            url += "&n=Custom+Functions&ncallerop=0&qi=x"
        }
        url += "&skip=" + this.state.page*this.state.size;
        url += "&pages=" + this.state.size;
        url += this.state.rev ? "&rev=1" : "";
        Axios.get(global.address + "xfunquery.html?" + url)
        .then((response) => {
            if (response.data.error) {
                this.setState({
                    error: response.data.error
                })
            } else {
                if (!response.data.f) {
                    this.setState({
                        f: [],
                        timer: response.data.timer,
                        loaded: true,
                        name: [],
                        start: 0,
                        info: [],
                        max: response.data.max,
                        ncallers: []
                    })
                } else
                    this.setState({
                        f: response.data.f,
                        timer: response.data.timer,
                        loaded: true,
                        info: response.data.funs.info,
                        start: 0,
                        max: response.data.max,
                        data: response.data,
                        ncallers: response.data.funs.ncallers
                    })
                console.log(this.state)
            }
        });
    } 

    showFs() {
        var toRender = [];
        var i;
        for (i = 0; i < this.state.size; i++) {
            if ((i) >= this.state.info.length) {
                break;
            }
            toRender.push(<tr key={i}>
                <td onDoubleClick={(e) => {
                    console.log(e.target.id)
                    this.props.changeType("fun", e.target.id)

                }}
                    id={this.state.f[i]} style={{ cursor: 'pointer' }}>{this.state.info[i]}</td>
                    <td>{this.state.ncallers[i]}</td>
            </tr>);
        }
        if(toRender.length === 0)
            toRender = <tr><td style={{border:'none', fontWeight:'bold'}}>No identifier found</td></tr>

        return toRender;
    }


    render() {
        return (
            <div>
                <h3>
                    Function Search
                </h3>
                <div className='searchFields'>
                    <form onSubmit={this.handleSubmit}>
                        <div className='textSearch'>
                            <input type='text' value={this.state.value} onChange={this.handleInputChange}
                                placeholder="Search..." /><br />
                        </div>
                    </form>
                    <form onSubmit={(e) => { 
                        e.preventDefault(); 
                        this.setState({ 
                            loaded: false
                        });  
                        console.log(e);
                        this.getFuns(); }}
                    >
                        <b>Type</b><br/>

                        <label className='chk'>
                            <input type='checkbox' className="chk" value='writable'
                                checked={this.state.writable} onChange={() => this.setState({writable: !this.state.writable})} 
                                />
                            Writable Declaration<br />
                            <span className='chk' />
                        </label>

                        <label className='chk'>
                            <input type='checkbox' className="chk" value='read-only'
                                checked={this.state.ro} onChange={() => this.setState({ro: !this.state.ro})} 
                                />
                            Read-Only Declaration<br />
                            <span className='chk' />
                        </label>

                        <label className='chk'>
                            <input type='checkbox' className="chk" value='cfun'
                                checked={this.state.cfun} onChange={() => this.setState({cfun: !this.state.cfun})} 
                                />
                            C functon<br />
                            <span className='chk' />
                        </label>

                        <button className="formButton" onClick={this.clearOptions}>Clear</button>
                        <button className="formButton" onClick={() => this.setState({match:"L"}) }>Search All</button>
                        <button className="formButton" onClick={() => this.setState({match:"Y"}) }>Search Any</button>
                    </form>
                    <form onSubmit={(e) => {
                        this.setState({
                            size: this.maxShow,
                            page: 0
                        }, this.getIds);
                        e.preventDefault()
                    }
                    }> Results per Page:
                        <input type='number' onChange={(e) => this.maxShow = e.target.value} min='1' max={this.state.loaded ? this.state.max : 200} /><br />
                    </form>
                </div>   
                {this.state.loaded ? 
                <table className="FileResults">
                    <thead>
                        <tr>
                            <td>
                                Function Name
                            </td>
                            <td>
                                Number of Callers
                            </td>
                        </tr>
                    </thead>
                    <tbody>
                        {this.showFs()}
                    </tbody>
                </table>
                :<div>Loading...</div>
                }
        </div>
        );
    }        
}
   
export default FunctionSearch;