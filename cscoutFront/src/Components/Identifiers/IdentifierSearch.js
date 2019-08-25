import Axios from 'axios';
import React, { Component } from 'react';
import '../../global.js';
import Uarr from '../asc.ico';
import Pager from '../Pager/Pager'

class IdentifierSearch extends Component {
    constructor(props) {
        super(props);
        this.state = {
            loaded: false,
            orderField: 0,
            rev: false,
            search: false,
            selectedOption: 'all',
            size: 20,
            page: 0,
            max:0,
            xfile: false
        }
        this.changeOrder = this.changeOrder.bind(this);
        this.handleSubmit = this.handleSubmit.bind(this);
        this.handleInputChange = this.handleInputChange.bind(this);
        this.handleOptionChange = this.handleOptionChange.bind(this);
        this.clearOptions = this.clearOptions.bind(this);
        this.inputValue = '';
        this.maxShow = 20;
        this.options = [];

    }

    componentDidMount() {
        this.getIds();
    }

    showPage() {
        var toRender = [];
     
        var i;
        for (i = 0; i < this.state.size; i++) {
            if ((i) >= this.state.info.length) {
                break;
            }
            toRender.push(<tr key={i}>
                <td onClick={(e) => {
                    console.log(e.target.id)
                    this.props.changeType("id", e.target.id)

                }}
                    id={this.state.ids[i]} style={{ cursor: 'pointer' }}>{this.state.info[i].name}</td>
                <td>{this.state.info[i].occ}</td>
            </tr>);
        }
        if(toRender.length === 0)
            toRender = <tr><td style={{border:'none', fontWeight:'bold'}}>No identifier found</td></tr>

        return toRender;
    }

    changeOrder(e) {
        if (this.state.orderField !== e) {
            this.setState({
                loaded: false,
                orderField: e,
                rev: false
            }, this.getIds)
        }
        else {
            if (this.state.rev) {
                this.setState({
                    rev: false,
                    orderField: 0
                }, this.getIds)

            }
            else
                this.setState({
                    rev: !this.state.rev
                }, this.getIds);
        }
    }

    getIds() {
        console.log(this.state);
        var url = "";
        url += (this.state.search) ?  "ire="+ this.inputValue  :  ""
        for(let i = 2; i < 16; i++)
            url += this.state["a" + i]?("&a" + i + "=1"):"";
        url += this.state.xfile?"&xfile=1":"";
        url += this.state.writable?"&writable=1":"";

       
        if(url === "")
            url = "writable=1&a2=1&match=Y&qi=1&n=All+Identifiers";
        else {
            if(this.state.match === undefined)
                url += "&match=L";
            else 
                url += "&match=" + this.state.match;
            url += "&qi=Custom+Identifiers"
        }
        url += "&skip=" + this.state.page*this.state.size;
        url += "&pages=" + this.state.size;
        url += this.state.rev ? "&rev=1" : "";
        url += (this.state.orderField === 2) ? "&qocc=1" : "";
        console.log(url);
        Axios.get(global.address + "xiquery?" + url)
            .then((response) => {
                if (response.data.error) {
                    this.setState({
                        error: response.data.error
                    })
                } else {
                    if (!response.data.id) {
                        this.setState({
                            ids: [],
                            timer: response.data.timer,
                            loaded: true,
                            name: [],
                            start: 0,
                            info: [],
                            max: response.data.max
                        })
                    } else
                        this.setState({
                            ids: response.data.id,
                            timer: response.data.timer,
                            loaded: true,
                            info: response.data.ids.info,
                            start: 0,
                            max: response.data.max
                        })
                    console.log(this.state)
                }
            });
    }

    handleSubmit(e) {
        e.preventDefault();

        this.setState({
            search: true
        }, this.getIds);
    }

    handleInputChange(e) {
        this.inputValue = e.target.value
    }

    handleOptionChange(e) {
        this.setState({
            selectedOption: e.target.value
        });
    }

    clearOptions() {
        for(let i = 2; i < 16; i++){
            this.setState({["a" + i]: false});
        }
        this.setState({
            writable: false,
            match: undefined
        })
    }

    render() {
        console.log(this.state)
        var opts = [
            "Read-Only",
            "Struct, union or enum",
            "Member of struct or union",
            "Label",
            "Ordinary identifier",
            "Macro",
            "Undefined macro",
            "Macro argument",
            "File scope",
            "Project scope",
            "Typedef",
            "Enumeration constant",
            "Yacc identifier",
            "Function"
        ];
        return (
            <div>
                <h3 className="titleSearch">
                    Identifier Search
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
                        this.getIds(); }}
                    >
                        <b>Type</b><br/>

                        <label className='chk'>
                            <input type='checkbox' className="chk" value='writable'
                                checked={this.state.writable} onChange={() => this.setState({writable: !this.state.writable})} 
                                />
                            Writable<br />
                            <span className='chk' />
                        </label>

                        {
                            opts.map((val,i) =>
                            <label className='chk' key={i}>
                                <input type='checkbox' className="chk" value={'a' + (i + 2)}
                                    onChange={() => this.setState({["a" + (i + 2)]: !this.state["a" + (i + 2)]})} 
                                    checked={this.state["a" + (i + 2)]}/>
                                {val}<br />
                                <span className='chk' />
                            </label>
                            )
                        }
                        <label className='chk'>
                            <input type='checkbox' className="chk" value='xfile'
                                onChange={() => this.setState({xfile: !this.state.xfile})} />
                            File-scoped<br />
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
                    <img src={Uarr} alt={'&#8593;'} align="right" style={(this.state.rev) ?
                        { transform: "scaleY(-1)" }
                        : {}
                    } />
    
                        <input type='number' onChange={(e) => this.maxShow = e.target.value} min='1' max={this.state.loaded ? this.state.max : 200} /><br />
                    </form>
                </div>
                {this.state.loaded ?
                    <div className="results">
                        <Pager setPage={(e) => {
                                this.setState({ page: e, start: e * this.state.size }, this.getIds);
                                }}
                            curPage={this.state.page} maxPage={this.state.max / this.state.size}
                            size={this.state.size} totalObjs={this.state.max} />
                        <table className="FileResults">
                            <colgroup>
                                <col style={{width:"70%"}}/>
                                <col style={{width:"30%"}}/>
                            </colgroup>
                            <thead>
                                <tr>
                                    <td onClick={() => { this.changeOrder(1); }}>
                                        Name
                                    {
                                            (this.state.orderField === 1) ?
                                                <img src={Uarr} alt={'&#8593;'} align="right" style={(this.state.rev) ?
                                                    { transform: "scaleY(-1)" }
                                                    : {}
                                                } />
                                                : ""
                                        }
                                    </td>
                                    <td onClick={() => { this.changeOrder(2); }}>
                                        Occurences
                                    {
                                            (this.state.orderField === 2) ?
                                                <img src={Uarr} alt={'&#8593;'} align="right" style={(this.state.rev) ?
                                                    { transform: "scaleY(-1)" }
                                                    : {}
                                                } />
                                                : ""
                                        }
                                    </td>
                                </tr>
                            </thead>
                            <tbody>
                                {this.showPage()}
                            </tbody>
                        </table>
                    </div>
                    : <div>Loading..</div>}
            </div>
        )
    }
}
export default IdentifierSearch;
