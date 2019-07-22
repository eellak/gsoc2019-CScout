import Axios from 'axios';
import React, { Component } from 'react';
import '../../global.js';
import Uarr from '../asc.ico';
import Pager from '../Files/FileSearch/Pager'

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
            max:0
        }
        this.changeOrder = this.changeOrder.bind(this);
        this.handleSubmit = this.handleSubmit.bind(this);
        this.handleInputChange = this.handleInputChange.bind(this);
        this.handleOptionChange = this.handleOptionChange.bind(this);
        this.inputValue = '';
        this.maxShow = 20;

    }

    componentDidMount() {
        this.getIds();
    }

    showPage() {
        var toRender = [];
        var start = this.state.page * this.state.size;
        var i;
        for (i = 0; i < this.state.size; i++) {
            if ((i) >= this.state.info.length) {
                break;
            }
            toRender.push(<tr key={i}>
                <td onDoubleClick={(e) => {
                    console.log(e.target.id)
                    this.props.changeType("id", e.target.id)

                }}
                    id={this.state.ids[i]} style={{ cursor: 'pointer' }}>{this.state.info[i].name}</td>
                <td>{this.state.info[i].occ}</td>
            </tr>);
        }

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
        var url;
        switch (this.state.selectedOption) {
            case ('all'):
                url = "writable=1&a2=1&match=Y&qi=1&n=All+Identifiers";
                break;
            case ('read-only'):
                url = "a2=1&match=Y&qi=1&n=Read-only+Identifiers";
                break;
            case ('writable'):
                url = "writable=1&match=Y&qi=1&n=Writable+Identifiers";
                break;
            default:
                url = "";
        }
        
        url += "&skip=" + this.state.page*this.state.size;
        url += "&pages=" + this.state.size;
        url += this.state.rev ? "&rev=1" : "";
        url += (this.state.orderField === 2) ? "&qocc=1" : "";
        url += (this.state.search) ?  "ire="+ this.inputValue  :  ""
        console.log(url);
        Axios.get(global.address + "xiquery.html?" + url)
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


    render() {
        console.log(this.state)
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
                    <form onSubmit={(e) => { this.setState({ loaded: false }); e.preventDefault(); this.getIds(); }}>
                        <label className='radioB'>
                            <input type='radio' className="type" value='all'
                                checked={this.state.selectedOption === 'all'} onChange={this.handleOptionChange} />
                            All<br />
                            <span className='checkmark' />
                        </label>
                        <label className='radioB'>
                            <input type='radio' className="type" value='writable'
                                checked={this.state.selectedOption === 'writable'} onChange={this.handleOptionChange} />
                            Writable<br />
                            <span class='checkmark' />
                        </label>
                        <label className='radioB'>
                            <input type='radio' className="type" value='read-only'
                                checked={this.state.selectedOption === 'read-only'} onChange={this.handleOptionChange} />
                            Read-Only<br />
                            <span class='checkmark' />
                        </label>
                        <button className="formButton">Submit</button>
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
                    <div className="results">
                        <Pager setPage={(e) => {
                                this.setState({ page: e, start: e * this.state.size }, this.getIds);
                                }}
                            curPage={this.state.page} maxPage={this.state.max / this.state.size}
                            size={this.state.size} totalObjs={this.state.max} />
                        <table className="FileResults">
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