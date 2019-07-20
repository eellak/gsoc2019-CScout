import React,{Component, Fragment} from 'react';
import Axios from 'axios';
import '../../../global.js';
import './FileSearch.css';
import Uarr from '../../asc.ico';
import Pager from './Pager';

class FileSearch extends Component{
    constructor(props){
        super(props);
        this.state = {
            loaded: false,
            size: 20,
            page: 0,
            orderby: "",
            orderField: 0,
            selectedOption:"all",
            rev: false           
        }
        this.objectComp = this.objectComp.bind(this);
        this.handleOptionChange = this.handleOptionChange.bind(this);
        this.handleSubmit = this.handleSubmit.bind(this);
        this.handleInputChange = this.handleInputChange.bind(this);
        this.inputValue = '';
        this.maxShow = 20;
    }


    componentDidMount(){
        this.getFiles();
        this.getSearchParams();
    }

    getSearchParams(){
        Axios.get(global.address + 'filesearch')
        .then((response) => 
            this.setState({
                options:response.data,
                optionsLoad: true
            })
        );
    }

    objectComp(a,b,e){
        if ( a[e] < b[e]){
            return -1;
        }
        if ( a[e] > b[e] ){
            return 1;
        }
        return 0;
    }

    orderTable(e){
        var files = this.state.show;           
        files.sort((a,b) => this.objectComp(a,b,e));
        this.setState({
            show: files
        })   
    }

    handleDoubleClick(){

    }
    
    showPage(){
        var  toRender = [];
        var start = this.state.page * this.state.size;
        var i;
        for(i = 0; i < this.state.size; i++){
            if((start + i) >= this.state.show.length){
                break;
            }
            console.log("id:"+this.state.show[start + i].id)
            toRender.push(<tr key={i}>
                <td onDoubleClick={(e) => {
                        console.log(e.target.id)
                        this.props.toFile("filePage",e.target.id)
                    
                    }} 
                    id={this.state.show[start+i].id} style={{cursor:'pointer'}}>{this.state.show[start + i].name}</td>
                <td>{this.state.show[start + i].path}</td>
                {this.state.metric?<td>{this.state.show[start + i].metric}</td>:null}
                </tr>);
        }
        
        return toRender;
    }

    changeOrder(e){ 
        if(this.state.orderField !== e){    
            console.log(e)
            console.log(Object.keys(this.state.files[0])[e])
            this.setState({
                orderby:Object.keys(this.state.files[0])[e],
                orderField:e,
                rev:false
            })
            this.orderTable(Object.keys(this.state.files[0])[e]);
        }
        else{
            if(this.state.rev){
                this.setState({
                    rev:false,
                    orderField:0
                })
                
            }
            else
                this.setState({
                    rev: !this.state.rev,
                    show: this.state.show.reverse()
                });
        }
    }

    pageNext(){
        this.setState({
            page: this.state.page+1
        })
    }
    pagePrev(){
        this.setState({
            page: this.state.page-1
        })
    }

    getFiles(){
        var url = this.props.customQ?this.props.customQ+"&":"";
        switch(this.state.selectedOption){
            case('all'):
                url = "ro=1&writable=1&match=Y&n=All+Files";
                break;
            case('read-only'):
                url = "ro=1&match=Y&n=Read-only+Files";
                break;
            case('writable'):
                url = "writable=1&match=Y&n=Writable+Files";
                break;
            case('unused-proj-scoped-id'):
                url = "writable=1&a11=1&unused=1&match=L&qf=1&n=Files+Containing+Unused+Project-scoped+Writable+Identifiers";
                break;
            case('unused-file-scoped-id'):
                url = "writable=1&a10=1&unused=1&match=L&qf=1&n=Files+Containing+Unused+File-scoped+Writable+Identifiers";
                break;
            case('wr-no-statement'):
                url = "writable=1&c16=1&n16=0&match=L&fre=%5C.%5BcC%5D%24&n=Writable+.c+Files+Without+Any+Statements";
                break;
            case('wr-unprocessed'):
                url = "writable=1&order=8&c8=4&n8=0&reverse=0&match=L&n=Writable+Files+Containing+Unprocessed+Lines";
                break;
            case('wr-strings'):
                url = "writable=1&c7=4&n7=0&match=L&n=Writable+Files+Containing+Strings";
                break;
            case('wr-h-include'):
                url = "writable=1&c"+this.state.options.nincfile+"="+this.state.options.gt+"&n"+this.state.options.nincfile+"=0&match=L&fre=%5C.%5BhH%5D%24&n=Writable+.h+Files+With+%23include+directives";
                break;
            default:
                break;
        }

        Axios.get(global.address + "xfilequery.html?" + url)
        .then((response) => {
            if(response.data.error){
                this.setState({
                    error: response.data.error
                })
            } else
            {
                if(!response.data.file){
                    this.setState({
                        files: [],
                        metric: response.metric,
                        timer: response.data.timer,
                        xfilequery: response.data.xfilequery,
                        loaded:true,
                        show: [],
                        size: 20,
                        start: 0
                    })
                } else 
                this.setState({
                    files: response.data.file,
                    metric: response.metric,
                    timer: response.data.timer,
                    xfilequery: response.data.xfilequery,
                    loaded:true,
                    show: response.data.file,
                    size: 20,
                    start: 0
                })                   

            }
        });
    }

    handleOptionChange(e) {
        this.setState({
          selectedOption: e.target.value
        });
      }

    handleSubmit(e) {
       
       this.setState({
           show: this.state.files.filter(
            x => x.name.includes(this.inputValue)
            )
          });
        e.preventDefault();
        console.log(this.state.show)
    }



    handleInputChange(e){
        this.inputValue = e.target.value

    }

    render(){

        return(
            <div>
                <h3 className="titleSearch">
                    File Search
                </h3>
                
                <div className='searchFields'>
                    <form onSubmit={this.handleSubmit}>
                        <div className='textSearch'>
                            <input type='text' value={this.state.value} onChange={this.handleInputChange} 
                            placeholder="Search..." /><br/>
                        </div>
                    </form>
                    <form onSubmit={(e) => {this.setState({loaded:false}); e.preventDefault(); this.getFiles();}}>
                        <label className='radioB'>
                        <input type='radio' className="type" value='all' 
                            checked={this.state.selectedOption === 'all'} onChange={this.handleOptionChange} />
                            All<br/>
                        <span class='checkmark'/>
                        </label>
                        <label className='radioB'>
                        <input type='radio' className="type" value='writable' 
                            checked={this.state.selectedOption === 'writable'} onChange={this.handleOptionChange}/>
                            Writable<br/>
                            <span class='checkmark'/>
                        </label>
                        <label className='radioB'>
                        <input type='radio' className="type" value='read-only' 
                            checked={this.state.selectedOption === 'read-only'} onChange={this.handleOptionChange}/>
                            Read-Only<br/>
                            <span class='checkmark'/>
                        </label>
                        <label className='radioB'>    
                        <input type='radio' className="type" value='unused-proj-scoped-id' 
                            checked={this.state.selectedOption === 'unused-proj-scoped-id'} onChange={this.handleOptionChange}/>
                            Files with unused project-scoped writable identifiers<br/>
                            <span class='checkmark'/>
                        </label>
                        <label className='radioB'>
                        <input type='radio' className="type" value='unused-file-scoped-id' 
                            checked={this.state.selectedOption === 'unused-file-scoped-id'} onChange={this.handleOptionChange}/>
                            Files with unused file-scoped writable identifiers<br/>
                            <span class='checkmark'/>
                        </label>
                        <label className='radioB'>
                        <input type='radio' className="type" value='wr-no-statement' 
                            checked={this.state.selectedOption === 'wr-no-statement'} onChange={this.handleOptionChange}/>
                            Writable Files without any statements<br/>       
                            <span class='checkmark'/>
                        </label>
                        <label className='radioB'>
                        <input type='radio' className="type" value='wr-unprocessed' 
                            checked={this.state.selectedOption === 'wr-unprocessed'} onChange={this.handleOptionChange}/>
                            Writable Files with unprocessed lines<br/>       
                            <span class='checkmark'/>
                        </label>
                        <label className='radioB'>
                        <input type='radio' className="type" value='wr-strings' 
                            checked={this.state.selectedOption === 'wr-strings'} onChange={this.handleOptionChange}/>
                            Writable Files with strings<br/> 
                            <span class='checkmark'/>
                        </label>
                        <label className='radioB'>
                        <input type='radio' className="type" value='wr-h-include' 
                            checked={this.state.selectedOption === 'wr-h-include'} onChange={this.handleOptionChange}/>
                            Writable .h Files with #include directives<br/>           
                            <span class='checkmark'/>
                        </label>
                        <button className="formButton">Submit</button>
                    </form>
                    <form onSubmit={(e)=> {
                        this.setState({
                            size:this.maxShow,
                            page:0
                        });
                        e.preventDefault()}
                        }> Results per Page:
                        <input type='number' onChange={(e) => this.maxShow=e.target.value} min='1' max={this.state.loaded?this.state.show.length:200}/><br/>
                    </form>
                   
                </div>
               
              
                {this.state.loaded?
                <div className="results">
                     <Pager setPage={(e) => this.setState({page:e,start:e*this.state.size})} 
                     curPage={this.state.page} maxPage={this.state.show.length / this.state.size}
                     size={this.state.size} totalObjs={this.state.show.length}/>
                    <table className="FileResults">
                        <thead>
                            <tr>
                                <td onClick={() => {this.changeOrder(1)}}>
                                    Name
                                    {
                                        (this.state.orderField === 1)?
                                            <img src={Uarr} align="right" style={(this.state.rev)?
                                            {transform: "scaleY(-1)"}
                                            :{}
                                            }/>
                                        :""                                   
                                    }
                                </td>
                                <td onClick={() => {this.changeOrder(2)}}>
                                    Path
                                    {
                                        (this.state.orderField === 2)?
                                            <img src={Uarr} align="right" style={(this.state.rev)?
                                            {transform: "scaleY(-1)"}
                                            :{}
                                            }/>
                                        :""                                   
                                    }
                                </td>
                                {
                                    this.state.metric?
                                    <td>Metric</td>
                                    :null
                                }
                            </tr>
                        </thead>
                        <tbody>
                        {                        
                            this.showPage()
                        }
                        </tbody>
                    </table>
                   
                </div> 
                :<div>Loading..</div>
                }
              
            </div>
        )
    }
}
export default FileSearch;