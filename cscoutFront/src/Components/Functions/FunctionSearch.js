import React,{Component} from 'react';
import Axios from 'axios';
import '../../global.js';
import Uarr from '../asc.ico';
import Pager from '../Pager/Pager';
import MetricParamSearch from '../MetricParamSearch';


class FunctionSearch extends Component {
    constructor(props) {
        super(props);
        this.state = {
            loaded: false,
            size: 20,
            page: 0,
            max: 0,
            orderby: "",
            orderField: 0,
            selectedOption: "all",
            rev: true
        }
        // this.objectComp = this.objectComp.bind(this);
        // this.handleOptionChange = this.handleOptionChange.bind(this);
        this.handleSubmit = this.handleSubmit.bind(this);
        this.handleInputChange = this.handleInputChange.bind(this);
        this.inputValue = '';
        this.maxShow = 20;
        this.selectedMetrics = [];
        this.changeSelectedMetrics = this.changeSelectedMetrics.bind(this)
    }

    componentDidMount() {
        this.getFuns();
    }

    changeSelectedMetrics(obj){
        this.selectedMetrics = obj;
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


    handleInputChange(e) {
        this.inputValue = e.target.value
    }


    getFuns() {
        console.log(this.state)
        console.log(this.selectedMetrics)
        var url = "";
        url += (this.state.search) ?  "fnre="+ this.inputValue  :  ""

        url += this.state.writable?"&writable=1":"";
        url += this.state.ro?"&ro=1":"";
        url += this.state.cfun?"&cfun=1":"";
        url += this.state.macro?"&macro=1":"";
        url += this.state.defined?"&defined=1":"";

        if(this.props.url) url = this.props.url;
        if(url === "")
            url = "writable=1&ro=1&match=Y&ncallerop=0&n=All+Functions&qi=x";
        else {
            if(this.state.match === undefined)
                url += "&match=L";
            else 
                url += "&match=" + this.state.match;
            url += "&n=Custom+Functions&ncallerop=0&qi=x"
        }

        url += this.state.pscope?"&pscope=1":"";
        url += this.state.fscope?"&fscope=1":"";
        url += "&skip=" + this.state.page*this.state.size;
        url += "&pages=" + this.state.size;
        url += this.state.rev ? "&reverse=1" : "";
        if(this.state.orderField === 2) 
            url += "&qncall=1" ;
        else if(this.state.orderField > 2)
            url += "&order=" + this.selectedMetrics[this.state.orderField - 3].val;
        this.selectedMetrics.map((obj,i) =>
            url += "&s" + obj.val + "=1"
        )
        if(this.selectedMetrics.length > 0)
            url += "&qmetr=1"
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
                        ncallers: [],
                        metrics: undefined
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
                        ncallers: response.data.funs.ncallers,
                        metrics: response.data.funs.metrics
                    })
                console.log(this.state)
            }
        });
    } 

    showMetricVals(i){
        if (this.state.metrics === undefined)
            return [];
        else
            return this.state.metrics[i].map((obj,i) =>
                            <td key={i}>
                                {obj}
                            </td>
                    );
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
                    this.props.changeType("fun", e.target.id)

                }}
                    id={this.state.f[i]} style={{ cursor: 'pointer' }}>{this.state.info[i]}</td>
                    <td>{this.state.ncallers[i]}</td>
                {
                    this.showMetricVals(i)
                }
            </tr>);
        }
        if(toRender.length === 0)
            toRender = <tr><td style={{border:'none', fontWeight:'bold'}}>No identifier found</td></tr>

        return toRender;
    }

    handleSubmit(e) {
        e.preventDefault();

        this.setState({
            search: true,
            orderField: 0
        }, this.getFuns);
    }

    showMetricHeaders() {
        if (this.state.metrics === undefined)
            return [];
        else 
            return this.selectedMetrics.map((obj,i) =>
                <td key={i} onClick={() => this.changeOrder(i+3)}>
                    {obj.name}
                    {
                        (this.state.orderField === (i+3)) ?
                            <img src={Uarr} alt={'&#8593;'} align="right" style={(this.state.rev) ?
                                { transform: "scaleY(-1)" }
                                : {}
                            } />
                            : ""
                    }
                </td>
            )
    }
    


    render() {
        var metrics = [
            "Number of characters",
            "Number of comment characters",
            "Number of space characters",
            "Number of line comments",
            "Number of block comments",
            "Number of lines",
            "Maximum number of characters in a line",
            "Number of character strings",
            "Number of unprocessed lines",
            "Number of C preprocessor directives",
            "Number of processed C preprocessor conditionals (ifdef, if, elif)",
            "Number of defined C preprocessor function-like macros",
            "Number of defined C preprocessor object-like macros",
            "Number of preprocessed tokens",
            "Number of compiled tokens",
            "Number of statements or declarations",
            "Number of operatiors"
        ]
        if(this.state.metrics !== undefined)
            this.selectedMetrics.sort();
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
                        this.getFuns(); }}
                    >
                        <b>Type</b><br/>

                        {(!this.props.url)?<div>
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
                       
                        <label className='chk'>
                            <input type='checkbox' className="chk" value='pscope'
                                checked={this.state.pscope} onChange={() => this.setState({pscope: !this.state.pscope})} 
                                />
                            Project Scope<br />
                            <span className='chk' />
                        </label>
                        <label className='chk'>
                            <input type='checkbox' className="chk" value='fscope'
                                checked={this.state.fscope} onChange={() => this.setState({fscope: !this.state.fscope})} 
                                />
                            File Scope<br />
                            <span className='chk' />
                        </label>
                        <label className='chk'>
                            <input type='checkbox' className="chk" value='macro'
                                checked={this.state.macro} onChange={() => this.setState({macro: !this.state.macro})} 
                                />
                            Function-like Macro<br />
                            <span className='chk' />
                        </label>
                        <label className='chk'>
                            <input type='checkbox' className="chk" value='defined'
                                checked={this.state.defined} onChange={() => this.setState({defined: !this.state.defined})} 
                                />
                            Defined<br />
                            <span className='chk' />
                        </label>
                        </div>
                        :<div>
                            <label className='chk'>
                                <input type='checkbox' className="chk" value='pscope'
                                    checked={this.state.pscope} onChange={() => this.setState({pscope: !this.state.pscope})} 
                                    />
                                Project Scope<br />
                                <span className='chk' />
                            </label>
                            <label className='chk'>
                                <input type='checkbox' className="chk" value='fscope'
                                    checked={this.state.fscope} onChange={() => this.setState({fscope: !this.state.fscope})} 
                                    />
                                File Scope<br />
                                <span className='chk' />
                            </label>
                        </div>}

                        <MetricParamSearch metrics={metrics} changeMetrics={this.changeSelectedMetrics}/>

                        <button className="formButton" onClick={this.clearOptions}>Clear</button>
                        <button className="formButton" onClick={() => this.setState({match:"L",orderField:0}) }>Search All</button>
                        <button className="formButton" onClick={() => this.setState({match:"Y",orderField:0}) }>Search Any</button>
                    </form>
                    <form onSubmit={(e) => {
                        this.setState({
                            size: this.maxShow,
                            page: 0
                        }, this.getFuns);
                        e.preventDefault()
                    }
                    }> Results per Page:
                        <input type='number' onChange={(e) => this.maxShow = e.target.value} min='1' max={this.state.loaded ? this.state.max : 200} /><br />
                    </form>
                </div> 
                {this.state.loaded ? 
                <div className="results" >
                    <Pager setPage={(e) => {
                            this.setState({ page: e, start: e * this.state.size }, this.getFuns);
                            }}
                        curPage={this.state.page} maxPage={this.state.max / this.state.size}
                        size={this.state.size} totalObjs={this.state.max} />
            
                    <table className="FileResults">
                        <thead>
                            <tr>
                                <td  onClick={() => { this.changeOrder(1); }} style={{minWidth:"20%"}}>
                                    Function Name
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
                                    Number of Callers
                                    {
                                            (this.state.orderField === 2) ?
                                                <img src={Uarr} alt={'&#8593;'} align="right" style={(this.state.rev) ?
                                                    { transform: "scaleY(-1)" }
                                                    : {}
                                                } />
                                                : ""
                                        }
                                </td>
                                {
                                    this.showMetricHeaders()
                                }
                            </tr>
                        </thead>
                        <tbody>
                            {this.showFs()}
                        </tbody>
                    </table>
                </div>
                :<div>Loading...</div>
                    }
        </div>
        );
    }        
}
   
export default FunctionSearch;