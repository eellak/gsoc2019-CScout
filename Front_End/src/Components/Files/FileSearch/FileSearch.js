import React,{Component, Fragment} from 'react';
import Axios from 'axios';
import '../../../global.js';
import './FileSearch.css'

class FileSearch extends Component{
    constructor(props){
        super(props);
        this.state = {
            loaded: false,
            size: 20,
            page: 0,
            orderby: 0,
            orderField: 0,
            selectedOption:"all"           
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
    }

    objectComp(a,b){
        if ( a[this.state.orderby] < b[this.state.orderby]){
            return -1;
        }
        if ( a[this.state.orderby] > b[this.state.orderby] ){
            return 1;
        }
        return 0;
    }

    orderTable(){
        var files = this.state.show;     
        files.sort(this.objectComp);
        this.setState({
            show: files
        })   
    }

    showPage(props){
        var  toRender = [];
        var start = this.state.page * this.state.size;
        var i;
        for(i = 0; i < this.state.size; i++){
            if((start + i) >= props.length){
                break;
            }
            toRender.push(<tr key={i}>
                <td>{props[i].name}</td>
                <td>{props[i].path}</td>
                {this.state.metric?<td>{props[i].metric}</td>:null}
                </tr>);
        }
        
        return toRender;
    }

    changeOrder(e){     
        this.setState({
            orderby:Object.keys(this.state.files[0])[e],
            orderField:e
        })
        this.orderTable();
    }

    pageNext(){
        this.setState({
            page: this.state.page+1
        })
    }

    getFiles(){
        var url;
        switch(this.state.selectedOption){
            case('all'):
                url = "ro=1&writable=1&match=Y&n=All+Files";
                break;
            case('read-only'):
                url = "ro=1&match=Y&n=Read-only+Files";
                break;
            case('writable'):
                url = "writable=1&match=Y&n=Writable+Files"
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
                    <form>
                        <input type='radio' className="type" value='all' 
                        checked={this.state.selectedOption === 'all'} onChange={this.handleOptionChange} />
                            All<br/>
                        <input type='radio' className="type" value='writable' 
                        checked={this.state.selectedOption === 'writable'} onChange={this.handleOptionChange}/>
                            Writable<br/>
                        <input type='radio' className="type" value='read-only' 
                        checked={this.state.selectedOption === 'read-only'} onChange={this.handleOptionChange}/>
                            Read-Only<br/>
                        {//<button>Submit</button>
                        }
                    </form>
                    <form onSubmit={(e)=> {
                        this.setState({
                            size:this.maxShow
                        });
                        e.preventDefault()}
                        }> Results per Page:
                        <input type='number' onChange={(e) => this.maxShow=e.target.value}/><br/>
                    </form>
                    <button onClick={() => this.changeOrder((this.state.orderField + 1)%2)}>Change Order</button>
                </div>
               
                {this.state.loaded?
                <table className="FileResults">
                    <thead>
                        <tr>
                            <td>
                                Name
                            </td>
                            <td>
                                Path
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
                        this.showPage(this.state.show)
                    }
                    </tbody>
                </table>
                :<div>Loading..</div>

                }
            </div>
        )
    }
}
export default FileSearch;