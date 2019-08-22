import React,{Component} from 'react';
import Axios from 'axios';

class FunList extends Component{
    constructor(props){
        super(props)
        this.state = {
            loaded:false,
            all: true
        }
        this.showList = this.showList.bind(this);
    }

    componentDidMount(){
        this.getFunList();
    }


    componentDidUpdate(prevProps){
        if (prevProps !== this.props) {
            this.getFunList()
        }
    }
    
    getFunList(){
        console.log(this.props)
        var query = this.state.all?this.props.n.toUpperCase():(this.props.n + "&e=1");
        console.log(global.address + "funlist?f=" + this.props.f + "&n=" + query)
        Axios.get(global.address + "funlist?f=" + this.props.f + "&n=" + query)
        .then((response) => {
            if(response.data.error){
                console.log(response.data.error)
            }
            else{
                this.setState({
                    loaded:true,
                    funs: response.data
                }, () =>  console.log(this.state)
                )
            }
        })
    }

    showList(prop){
        var to_Render = [];
        if(prop !== null && Array.isArray(prop))
            prop.map((x,i) => to_Render.push(
                <FunItem x={x} changeType={this.props.changeType} key={i} recurse={this.showList}/>)
            // <li key={i}>
            // <a onClick={() => {
            //     console.log(x);this.props.changeType("fun",x.f)
            //     }}> {"\u21B3"}{x.fname} </a>
            //     {x.call?
            //     <ul >
            //         {this.showList(x.call)}
            //     </ul>
            //     :null}
            // </li>)
            );
        else
                    to_Render = <div>No functions</div>
        return to_Render;
    }

    render(){
        console.log(this.state)
        return(
            <div>
                {/* <button onClick={() => this.setState({all: !this.state.all,loaded:false}, this.getFunList)}>Direct</button> */}
                <ul className="lists">
                    {this.state.loaded?this.showList(this.state.funs.funs):
                    <div>Loading...</div>}
                </ul>
            </div>
        )
    }
}

class FunItem extends Component{
    constructor(props) {
        super(props)
        this.state = {
            expand:false
        }
    }

    render(){
        return(
            <li className="flist">
                <a onClick={() => this.setState({expand:!this.state.expand}) }> {this.state.expand?"\u25BE":"\u25B8"}</a>
                <a onClick={() => {
                    console.log(this.props.x);this.props.changeType("fun",this.props.x.f)
                }}> {this.props.x.fname} </a>
                <hr/>
                {(this.props.x.call && this.state.expand)?
                    <ul>{this.props.recurse(this.props.x.call)}</ul>
                :null}
            </li>
        )
    }
}


export default FunList;