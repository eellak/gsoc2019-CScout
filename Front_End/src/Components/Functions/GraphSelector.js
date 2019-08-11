import React,{Component} from 'react';
import ReactSVG from 'react-svg';
import '../../global.js';

class GraphControl extends Component{
    constructor(props){
        super(props);
        this.state = {
            optionsState:'B'
        }
    }

    change(event){
        console.log(event);
        this.setState({
            optionsState: event.target.value
        })
        event.preventDefault();
    }

    contentClickHandler = (e) => {
        const targetLink = e.target.closest('a');
        if(!targetLink) return;
        console.log(targetLink.getAttribute("xlink:href"));
        this.props.changeType("fun",targetLink.getAttribute("xlink:href"));
        e.preventDefault();
        

    render(){
        return(
            <div className="selector">
                <select value={this.state.optionsState} onChange={this.change.bind(this)}>
                    <option value='B' className="selected">All calling and called</option>
                    <option value='U'>All Callers</option>
                    <option value='D'>All called</option>
                </select>
                <ReactSVG src={global.address + "cgraph.svg?all=1&n=" + this.state.optionsState + "&f=" + this.props.f} 
                    onClick={this.contentClickHandler}  className="svgCont"/>
            </div>               
        )
    }
}

export default GraphControl;        
        
        
        
        
        
      