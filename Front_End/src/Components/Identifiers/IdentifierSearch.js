import React,{Component} from 'react';
import Axios from 'axios';
import '../../global.js';
import IdDependancies from './IdDependancies';
import Uarr from '../asc.ico';

class IdentifierSearch extends Component{
    constructor(props){
        super(props);
        this.state = {
            loaded:false,
            orderField:0,
            rev:false
        }
        this.changeOrder = this.changeOrder.bind(this);
    }

    componentDidMount(){
        this.getIds();
    }

    showPage(){
        var  toRender = [];
        var start = this.state.page * this.state.size;
        var i;
        for(i = 0; i < this.state.size; i++){
            if((i) >= this.state.info.length){
                break;
            }
            // console.log(this.state.show[start+i])
            toRender.push(<tr key={i}>
                <td onDoubleClick={(e) => {
                        console.log(e.target.id)
                        this.props.changeType("filePage",e.target.id)
                    
                    }} 
                    id={this.state.ids[i]} style={{cursor:'pointer'}}>{this.state.info[i].name}</td>
                <td>{this.state.info[i].occ}</td>
                </tr>);
        }
        
        return toRender;
    }





    changeOrder(e){ 
        console.log(e)
        console.log(this.state)
        if(this.state.orderField !== e){    
            this.setState({
                loaded:false,
                orderField:e,
                rev:false
            },this.getIds)
            console.log(this.state)
        }
        else{
            console.log("here"+e)
            if(this.state.rev){
                this.setState({
                    rev:false,
                    orderField:0
                },this.getIds)
                
            }
            else
                this.setState({
                    rev: !this.state.rev
                },this.getIds);
        }
        console.log(this.state)
    }
    

    getIds(){
        console.log(this.state);
        var url = this.state.rev ? "&rev=1":"";
        url += (this.state.orderField === 2)?"&qocc=1":"";
        console.log(url);
        Axios.get(global.address + "xiquery.html?"+"writable=1&a2=1&match=Y&qi=1&n=All+Identifiers"+url)
        .then((response) => {
            if(response.data.error){
                this.setState({
                    error: response.data.error
                })
            } else
            {
                if(!response.data.id){
                    this.setState({
                        ids: [],
                        timer: response.data.timer,
                        loaded:true,
                        name: [],
                        size: 20,
                        start: 0,
                        page:0
                    })
                } else 
                this.setState({
                    ids: response.data.id,
                    timer: response.data.timer,
                    loaded:true,
                    info: response.data.ids.info,
                    size: 20,
                    start: 0,
                    page:0
                })                   
                console.log(this.state)
            }
        });
    }
    
    render(){
        console.log(this.state)
        return(
            <div>
               {this.state.loaded?
                <table className="FileResults">
                    <thead>
                        <tr>
                            <td onClick={() => {this.changeOrder(1);}}>
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
                            <td onClick={() => {this.changeOrder(2);}}>
                                Occurences
                                {
                                    (this.state.orderField === 2)?
                                        <img src={Uarr} align="right" style={(this.state.rev)?
                                        {transform: "scaleY(-1)"}
                                        :{}
                                        }/>
                                    :""                                   
                                }
                            </td>
                        </tr>
                    </thead>
                    <tbody>
                    {this.showPage()}
                    </tbody>

                </table>
                :<div>Loading..</div> }
            </div>
        )
    }
}
export default IdentifierSearch;